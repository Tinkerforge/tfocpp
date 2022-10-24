#pragma once

#include "Platform.h"
#include "Configuration.h"

#include <memory>

class OcppChargePoint;

struct ValueToSend {
    time_t timestamp;
    std::unique_ptr<MeterValueSampledValue[]> sampled_values;
    size_t sampled_value_count;
    std::unique_ptr<char[]> sampled_value_content;
};

struct MeterValueAccumulator {
    void init(int32_t connId, OcppChargePoint *chargePoint, ConfigKey data_to_sample);
    void reset();
    void tick();
    std::unique_ptr<float[]> meter_values = nullptr;
    std::unique_ptr<SampledValueMeasurand[]> measurands = nullptr;
    size_t measurand_count;
    size_t supported_measurand_count;
    size_t meter_values_len = 0;
    uint32_t samples_this_run = 0;

    ValueToSend get(SampledValueContext context);

    int connectorId;
    ConfigKey data_to_sample;
    OcppChargePoint *cp;
};
