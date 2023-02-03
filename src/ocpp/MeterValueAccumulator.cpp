#include "MeterValueAccumulator.h"
#include "Tools.h"
#include "Configuration.h"
#include "ChargePoint.h"
#include "Platform.h"

enum class MeasurandType {
    Register,
    Interval,
    Average
};

static MeasurandType get_measurand_type(SampledValueMeasurand m, bool average) {
    switch (m) {
        case SampledValueMeasurand::ENERGY_ACTIVE_EXPORT_REGISTER:
        case SampledValueMeasurand::ENERGY_ACTIVE_IMPORT_REGISTER:
        case SampledValueMeasurand::ENERGY_REACTIVE_EXPORT_REGISTER:
        case SampledValueMeasurand::ENERGY_REACTIVE_IMPORT_REGISTER:
            return MeasurandType::Register;

        case SampledValueMeasurand::ENERGY_ACTIVE_EXPORT_INTERVAL:
        case SampledValueMeasurand::ENERGY_ACTIVE_IMPORT_INTERVAL:
        case SampledValueMeasurand::ENERGY_REACTIVE_EXPORT_INTERVAL:
        case SampledValueMeasurand::ENERGY_REACTIVE_IMPORT_INTERVAL:
            return MeasurandType::Interval;

        case SampledValueMeasurand::POWER_ACTIVE_EXPORT:
        case SampledValueMeasurand::POWER_ACTIVE_IMPORT:
        case SampledValueMeasurand::POWER_OFFERED:
        case SampledValueMeasurand::POWER_REACTIVE_EXPORT:
        case SampledValueMeasurand::POWER_REACTIVE_IMPORT:
        case SampledValueMeasurand::POWER_FACTOR:
        case SampledValueMeasurand::CURRENT_IMPORT:
        case SampledValueMeasurand::CURRENT_EXPORT:
        case SampledValueMeasurand::CURRENT_OFFERED:
        case SampledValueMeasurand::VOLTAGE:
        case SampledValueMeasurand::FREQUENCY:
        case SampledValueMeasurand::TEMPERATURE:
        case SampledValueMeasurand::SO_C:
        case SampledValueMeasurand::RPM:
            return average ? MeasurandType::Average : MeasurandType::Register;

        case SampledValueMeasurand::NONE:
            return MeasurandType::Register;
    }
}

// Assert that _REGISTER is always 4 before the equivalent _INTERVAL value.
static_assert((size_t)SampledValueMeasurand::ENERGY_ACTIVE_EXPORT_REGISTER + 4 == (size_t)SampledValueMeasurand::ENERGY_ACTIVE_EXPORT_INTERVAL, "_REGISTER value block is assumed to be directly behind _INTERVAL value block");
static_assert((size_t)SampledValueMeasurand::ENERGY_ACTIVE_IMPORT_REGISTER + 4 == (size_t)SampledValueMeasurand::ENERGY_ACTIVE_IMPORT_INTERVAL, "_REGISTER value block is assumed to be directly behind _INTERVAL value block");
static_assert((size_t)SampledValueMeasurand::ENERGY_REACTIVE_EXPORT_REGISTER + 4 == (size_t)SampledValueMeasurand::ENERGY_REACTIVE_EXPORT_INTERVAL, "_REGISTER value block is assumed to be directly behind _INTERVAL value block");
static_assert((size_t)SampledValueMeasurand::ENERGY_REACTIVE_IMPORT_REGISTER + 4 == (size_t)SampledValueMeasurand::ENERGY_REACTIVE_IMPORT_INTERVAL, "_REGISTER value block is assumed to be directly behind _INTERVAL value block");

void MeterValueAccumulator::tick()
{
    size_t value_offset = 0;

    for(size_t i = 0; i < measurand_count; ++i) {
        auto measurand = measurands[i];
        auto measurand_type = get_measurand_type(measurand, this->average);
        // If we want an interval measurand, get the corresponding register value to calculate the interval with.
        // We should not query the platform for interval measurements.
        if (measurand_type == MeasurandType::Interval)
            measurand = (SampledValueMeasurand)((size_t)measurand - 4);

        size_t supported_count = platform_get_supported_measurand_count(this->connectorId, measurand);
        auto *supported = platform_get_supported_measurands(this->connectorId, measurand);

        for(size_t supported_idx = 0; supported_idx < supported_count; ++supported_idx) {
            auto s = supported[supported_idx];

            switch (measurand_type) {
                case MeasurandType::Register:
                    if (s.is_signed) {
                        log_warn("Signed values not supported yet!");
                        continue;
                    }

                    meter_values[value_offset] = platform_get_raw_meter_value(this->connectorId, measurand, s.phase, s.location);
                    break;
                case MeasurandType::Interval:
                    // Use two values for intervals as a mini ring-buffer. Set the first one only on the first run after boot-up.
                    // ::reset() "rotates" the ring buffer.
                    if (first_run)
                        meter_values[value_offset] = platform_get_raw_meter_value(this->connectorId, measurand, s.phase, s.location);
                    else
                        meter_values[value_offset + 1] = platform_get_raw_meter_value(this->connectorId, measurand, s.phase, s.location);
                    ++value_offset;
                    break;
                case MeasurandType::Average:
                    float new_value = platform_get_raw_meter_value(this->connectorId, measurand, s.phase, s.location);
                    // TODO: store value "undivided" here and divide by samples_this_run only in get()
                    meter_values[value_offset] = ((meter_values[value_offset] * samples_this_run) + new_value) / ((float)samples_this_run + 1);
                    break;
            }

            ++value_offset;
        }
    }
    first_run = false;

    ++samples_this_run;
}

ValueToSend MeterValueAccumulator::get(SampledValueContext context)
{
    // Sample a value per connector and phase.
    auto sampled_values = heap_alloc_array<MeterValueSampledValue>(supported_measurand_count);
    auto sampled_value_content = heap_alloc_array<char>(supported_measurand_count * OCPP_PLATFORM_MEASURAND_MAX_DATA_LEN);
    size_t sampled_value_idx = 0;
    size_t value_offset = 0;

    for(size_t i = 0; i < measurand_count; ++i) {
        auto measurand = measurands[i];
        auto measurand_type = get_measurand_type(measurand, this->average);
        if (measurand_type == MeasurandType::Interval)
            measurand = (SampledValueMeasurand)((size_t)measurand - 4);

        size_t count = platform_get_supported_measurand_count(this->connectorId, measurand);
        auto *sup = platform_get_supported_measurands(this->connectorId, measurand);

        for(size_t supported_idx = 0; supported_idx < count; ++supported_idx) {
            auto m = sup[supported_idx];
            bool is_signed = false;

            float value = meter_values[value_offset];
            if (measurand_type == MeasurandType::Interval) {
                // We need two samples in the first run (start and end of the interval)
                // and one additional sample per run after the first one
                // (end of the last interval becomes start of the next one)
                if ((first_run && samples_this_run == 0) || samples_this_run <= 1)
                    value = 0;
                else
                    value = meter_values[value_offset + 1] - value;
                ++value_offset;
            }

            char *value_buf = sampled_value_content.get() + (sampled_value_idx * OCPP_PLATFORM_MEASURAND_MAX_DATA_LEN);
            snprintf(value_buf, OCPP_PLATFORM_MEASURAND_MAX_DATA_LEN, "%.3f", value);

            sampled_values[sampled_value_idx].value = value_buf;
            sampled_values[sampled_value_idx].format = is_signed ? SampledValueFormat::SIGNED_DATA : SampledValueFormat::RAW;
            sampled_values[sampled_value_idx].location = m.location;
            sampled_values[sampled_value_idx].measurand = measurand_type == MeasurandType::Interval ? (SampledValueMeasurand)((size_t)measurand + 4) : measurand;
            sampled_values[sampled_value_idx].phase = m.phase;
            sampled_values[sampled_value_idx].unit = m.unit;
            sampled_values[sampled_value_idx].context = context;

            ++sampled_value_idx;
            ++value_offset;
        }
    }

    return ValueToSend{
        std::move(sampled_values),
        supported_measurand_count,
        std::move(sampled_value_content)};
}

void MeterValueAccumulator::reset()
{
    size_t value_offset = 0;

    for(size_t i = 0; i < measurand_count; ++i) {
        auto measurand = measurands[i];
        auto measurand_type = get_measurand_type(measurand, this->average);
        // If we want an interval measurand, get the corresponding register value to calculate the interval with.
        // We should not query the platform for interval measurements.
        if (measurand_type == MeasurandType::Interval)
            measurand = (SampledValueMeasurand)((size_t)measurand - 4);

        size_t supported_count = platform_get_supported_measurand_count(this->connectorId, measurand);

        for(size_t supported_idx = 0; supported_idx < supported_count; ++supported_idx) {
            switch (measurand_type) {
                case MeasurandType::Register:
                case MeasurandType::Average:
                    meter_values[value_offset] = 0;
                    break;
                case MeasurandType::Interval:
                    if (samples_this_run > 1)
                        meter_values[value_offset] = meter_values[value_offset + 1];
                    meter_values[value_offset + 1] = 0;
                    ++value_offset;
                    break;
            }

            ++value_offset;
        }
    }

    samples_this_run = 0;
}

void MeterValueAccumulator::init(int32_t connId, bool average, OcppChargePoint *chargePoint, ConfigKey dataToSample)
{
    this->connectorId = connId;
    this->average = average;
    this->cp = chargePoint;
    this->data_to_sample = dataToSample;

    meter_values_len = 0;

    measurand_count = getCSLConfigLen(dataToSample);
    supported_measurand_count = 0;
    size_t *measurands_configured = getCSLConfig(dataToSample);

    measurands = heap_alloc_array<SampledValueMeasurand>(measurand_count);
    for(size_t i = 0; i < measurand_count; ++i) {
        auto m = (SampledValueMeasurand)measurands_configured[i];
        measurands[i] = m; // Store the unmodified measurand to be able to disambiguate between _INTERVAL and _REGISTER later.

        auto type = get_measurand_type(m, this->average);
        // If we want an interval measurand, get the corresponding register value to calculate the interval with.
        // We should not query the platform for interval measurements.
        if (type == MeasurandType::Interval)
            m = (SampledValueMeasurand)((size_t)m - 4);

        size_t supported_count = platform_get_supported_measurand_count(this->connectorId, m);
        supported_measurand_count += supported_count;
        meter_values_len += supported_count * (type == MeasurandType::Interval ? 2 : 1);
    }

    meter_values = heap_alloc_array<float>(meter_values_len);
    reset();
}
