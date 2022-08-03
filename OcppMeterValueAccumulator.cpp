#include "OcppMeterValueAccumulator.h"
#include "OcppTools.h"
#include "OcppConfiguration.h"
#include "OcppChargePoint.h"

enum class MeasurandType {
    Register,
    Interval,
    Average
};

static MeasurandType get_measurand_type(SampledValueMeasurand m) {
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
            return MeasurandType::Average;

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

    size_t measurand_count = getCSLConfigLen(data_to_sample);
    size_t *measurands = getCSLConfig(data_to_sample);
    for(size_t measurand_idx = 0; measurand_idx < measurand_count; ++measurand_idx) {
        auto measurand = (SampledValueMeasurand)measurands[measurand_idx];
        auto measurand_type = get_measurand_type(measurand);
        // If we want an interval measurand, get the corresponding register value to calculate the interval with.
        // We should not query the platform for interval measurements.
        if (measurand_type == MeasurandType::Interval)
            measurand = (SampledValueMeasurand)measurands[measurand_idx - 4];

        size_t supported_count = platform_get_supported_measurand_count(this->connectorId, measurand);
        SupportedMeasurand *supported = platform_get_supported_measurands(this->connectorId, measurand);

        for(size_t supported_idx = 0; supported_idx < supported_count; ++supported_idx) {
            auto s = supported[supported_idx];

            switch (measurand_type) {
                case MeasurandType::Register:
                    if (s.is_signed) {
                        platform_printfln("Signed values not supported yet!");
                        continue;
                    }

                    meter_values[value_offset] = platform_get_raw_meter_value(this->connectorId, measurand, s.phase, s.location);
                    break;
                case MeasurandType::Interval:
                    // Use two values for intervals: one for the first run after the interval reset, one for the latest value
                    // We can then calculate the interval value when sending.
                    if (samples_this_run == 0)
                        meter_values[value_offset] = platform_get_raw_meter_value(this->connectorId, measurand, s.phase, s.location);
                    else
                        meter_values[value_offset + 1] = platform_get_raw_meter_value(this->connectorId, measurand, s.phase, s.location);
                    ++value_offset;
                    break;
                case MeasurandType::Average:
                    float new_value = platform_get_raw_meter_value(this->connectorId, measurand, s.phase, s.location);
                    meter_values[value_offset] = ((meter_values[value_offset] * samples_this_run) + new_value) / ((float)samples_this_run + 1);
                    break;
            }

            ++value_offset;
        }
    }

    ++samples_this_run;
}

ValueToSend MeterValueAccumulator::get(SampledValueContext context)
{
    size_t measurand_count = getCSLConfigLen(data_to_sample);
    size_t *measurands = getCSLConfig(data_to_sample);

    size_t sampled_value_count = 0;

    for(size_t measurand_idx = 0; measurand_idx < measurand_count; ++measurand_idx) {
        auto measurand = (SampledValueMeasurand)measurands[measurand_idx];
        if (get_measurand_type(measurand) == MeasurandType::Interval)
            measurand = (SampledValueMeasurand)measurands[measurand_idx - 4];
        sampled_value_count += platform_get_supported_measurand_count(this->connectorId, measurand);
    }

    // No need to handle sampled_value_count == 0 here: If sampled_value_count is 0, get will not be called anyway.

    // Sample a value per connector and phase.
    auto sampled_values = std::unique_ptr<MeterValueSampledValue[]>(new MeterValueSampledValue[sampled_value_count]);
    auto sampled_value_content = std::unique_ptr<char[]>(new char[sampled_value_count * PLATFORM_MEASURAND_MAX_DATA_LEN]);
    size_t sampled_value_idx = 0;
    size_t value_offset = 0;

    for(size_t i = 0; i < measurand_count; ++i) {
        SampledValueMeasurand measurand = (SampledValueMeasurand) measurands[i];
        auto measurand_type = get_measurand_type(measurand);
        if (measurand_type == MeasurandType::Interval)
            measurand = (SampledValueMeasurand)measurands[i - 4];

        size_t count = platform_get_supported_measurand_count(this->connectorId, measurand);
        SupportedMeasurand *sup = platform_get_supported_measurands(this->connectorId, measurand);

        for(size_t supported_idx = 0; supported_idx < count; ++supported_idx) {
            auto m = sup[supported_idx];
            bool is_signed = false;

            float value = meter_values[value_offset];
            if (measurand_type == MeasurandType::Interval) {
                value = meter_values[value_offset + 1] - value;
                ++value_offset;
            }

            char *value_buf = sampled_value_content.get() + (sampled_value_idx * PLATFORM_MEASURAND_MAX_DATA_LEN);
            snprintf(value_buf, PLATFORM_MEASURAND_MAX_DATA_LEN, "%.3f", value);

            sampled_values[sampled_value_idx].value = value_buf;
            sampled_values[sampled_value_idx].format = is_signed ? SampledValueFormat::SIGNED_DATA : SampledValueFormat::RAW;
            sampled_values[sampled_value_idx].location = m.location;
            sampled_values[sampled_value_idx].measurand = measurand;
            sampled_values[sampled_value_idx].phase = m.phase;
            sampled_values[sampled_value_idx].unit = m.unit;
            sampled_values[sampled_value_idx].context = context;

            ++sampled_value_idx;
            ++value_offset;
        }
    }

    return ValueToSend{
        platform_get_system_time(this->cp->platform_ctx),
        std::move(sampled_values),
        sampled_value_count,
        std::move(sampled_value_content)};
}

void MeterValueAccumulator::reset()
{
    memset(meter_values.get(), 0, sizeof(meter_values[0]) * meter_values_len);
    samples_this_run = 0;
}

void MeterValueAccumulator::init(int32_t connId, OcppChargePoint *chargePoint, ConfigKey dataToSample)
{
    this->connectorId = connId;
    this->cp = chargePoint;
    this->data_to_sample = dataToSample;

    meter_values_len = 0;

    size_t measurand_count = getCSLConfigLen(dataToSample);
    size_t *measurands = getCSLConfig(dataToSample);
    for(size_t measurand_idx = 0; measurand_idx < measurand_count; ++measurand_idx) {
        auto measurand = (SampledValueMeasurand)measurands[measurand_idx];
        auto measurand_type = get_measurand_type(measurand);
        // If we want an interval measurand, get the corresponding register value to calculate the interval with.
        // We should not query the platform for interval measurements.
        if (measurand_type == MeasurandType::Interval)
            measurand = (SampledValueMeasurand)measurands[measurand_idx - 4];

        size_t supported_count = platform_get_supported_measurand_count(this->connectorId, measurand);
        meter_values_len += supported_count * (measurand_type == MeasurandType::Interval ? 2 : 1);
    }

    meter_values = std::unique_ptr<float[]>(new float[meter_values_len]);
}
