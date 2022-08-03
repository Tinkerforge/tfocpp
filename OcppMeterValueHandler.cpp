#include "OcppMeterValueHandler.h"

#include "OcppChargePoint.h"

void OcppMeterValueHandler::tick() {
    // TODO: let the platform trigger when new values are available instead of polling.
    // This makes sure that meter values are always up to date if we also handle the clock aligned/interval sampling and sending here.
    if (!deadline_elapsed(last_run + PLATFORM_MEASURAND_ACQUISITION_INTERVAL_MS))
        return;

    last_run = platform_now_ms();

    if (getIntConfig(ConfigKey::ClockAlignedDataInterval) != 0)
        clock_aligned_meter_values.tick();

    if (getIntConfig(ConfigKey::MeterValueSampleInterval) != 0 && this->transaction_active())
        charging_session_meter_values.tick();

    if (clock_aligned_interval_crossed()) {
        auto to_send = clock_aligned_meter_values.get(SampledValueContext::SAMPLE_CLOCK);
        MeterValue mv;
        mv.timestamp = to_send.timestamp;
        mv.sampledValue = to_send.sampled_values.get();
        mv.sampledValue_length = to_send.sampled_value_count;
        cp->sendCallAction(CallAction::METER_VALUES, MeterValues(this->connectorId, &mv, 1));
        clock_aligned_meter_values.reset();
    }

    if (charging_session_interval_crossed()) {
        auto to_send = charging_session_meter_values.get(SampledValueContext::SAMPLE_PERIODIC);
        MeterValue mv;
        mv.timestamp = to_send.timestamp;
        mv.sampledValue = to_send.sampled_values.get();
        mv.sampledValue_length = to_send.sampled_value_count;
        cp->sendCallAction(CallAction::METER_VALUES, MeterValues(this->connectorId, &mv, 1));
        charging_session_meter_values.reset();
    }
}

bool OcppMeterValueHandler::clock_aligned_interval_crossed() {
    int32_t interval = getIntConfig(ConfigKey::ClockAlignedDataInterval);

    /*
    A value of "0" (numeric zero), by convention, is to be interpreted to mean that no clock-aligned data should be
    transmitted.
    */
    if (interval == 0)
        return false;

    time_t now = platform_get_system_time(cp->platform_ctx);
    struct tm last_send_tm;
    struct tm now_tm;

    gmtime_r(&last_clock_aligned_send, &last_send_tm);
    gmtime_r(&now, &now_tm);

    // If !=, we just crossed midnight. This is an interval start in any case, as the intervals are aligned at midnight.
    if (last_send_tm.tm_yday == now_tm.tm_yday){
        uint32_t last_send_seconds_since_midnight = last_send_tm.tm_hour * 3600 + last_send_tm.tm_min * 60 + last_send_tm.tm_sec;
        uint32_t now_seconds_since_midnight = now_tm.tm_hour * 3600 + now_tm.tm_min * 60 + now_tm.tm_sec;

        uint32_t last_send_interval = last_send_seconds_since_midnight / interval;
        uint32_t now_interval = now_seconds_since_midnight / interval;

        if (now_interval == last_send_interval)
            return false;
    }

    last_clock_aligned_send = now;
    return true;
}

bool OcppMeterValueHandler::charging_session_interval_crossed()
{
    int32_t interval = getIntConfig(ConfigKey::MeterValueSampleInterval);
    if (interval == 0)
        return false;

    if (!deadline_elapsed(last_charging_session_send + interval * 1000))
        return false;

    last_charging_session_send = platform_now_ms();
    return true;
}
