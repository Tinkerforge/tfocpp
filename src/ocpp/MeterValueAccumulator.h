#pragma once

#include "Configuration.h"

#include <memory>

class OcppChargePoint;

struct SupportedMeasurand;

struct ValueToSend {
    std::unique_ptr<MeterValueSampledValue[]> sampled_values;
    size_t sampled_value_count;
    std::unique_ptr<char[]> sampled_value_content;
};

struct MeterValueAccumulator {
    void init(int32_t connId, bool average_values, OcppChargePoint *chargePoint, ConfigKey data_to_sample);
    void initMeter();
    void reset();
    void tick();
    std::unique_ptr<float[]> meter_values = nullptr;

    std::unique_ptr<SampledValueMeasurand[]> measurands = nullptr;
    std::unique_ptr<SampledValuePhase[]> measurand_phases = nullptr;
    size_t measurand_count;

    void * platform_meter_cache = nullptr;

    SupportedMeasurand *supported_measurands = nullptr;
    size_t supported_measurand_count;

    uint32_t samples_this_run = 0;
    bool first_run = true;

    ValueToSend get(SampledValueContext context);

    int connectorId;
    bool average;
    ConfigKey data_to_sample;
    OcppChargePoint *cp;
};
