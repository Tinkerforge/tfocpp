#include "MeterValueHandler.h"

#include "ChargePoint.h"
#include "Platform.h"

void OcppMeterValueHandler::init(int32_t connId, OcppChargePoint *chargePoint) {
    this->connectorId = connId;
    this->cp = chargePoint;
    this->clock_aligned_meter_values.init(connId, true, chargePoint, ConfigKey::MeterValuesAlignedData);
    this->charging_session_meter_values.init(connId, false, chargePoint, ConfigKey::MeterValuesSampledData);
    last_clock_aligned_send_timestamp = platform_get_system_time(cp->platform_ctx);
}

void OcppMeterValueHandler::tick() {
    // TODO: let the platform trigger when new values are available instead of polling.
    // This makes sure that meter values are always up to date if we also handle the clock aligned/interval sampling and sending here.
    if (!deadline_elapsed(last_run + OCPP_PLATFORM_MEASURAND_ACQUISITION_INTERVAL_MS))
        return;

    last_run = platform_now_ms();

    if (getIntConfig(ConfigKey::ClockAlignedDataInterval) != 0)
        clock_aligned_meter_values.tick();

    if (getIntConfig(ConfigKey::MeterValueSampleInterval) != 0 && this->transaction_active() && charging_session_meter_values.samples_this_run == 0)
        charging_session_meter_values.tick();

    time_t timestamp;
    if (clock_aligned_interval_crossed(&timestamp)) {
        auto to_send = clock_aligned_meter_values.get(SampledValueContext::SAMPLE_CLOCK);
        MeterValue mv;
        mv.timestamp = timestamp;
        mv.sampledValue = to_send.sampled_values.get();
        mv.sampledValue_length = to_send.sampled_value_count;
        if (mv.sampledValue_length > 0) {
            log_info("Creating MeterValues.req of connector %d with %lu values (clock-aligned)", this->connectorId, mv.sampledValue_length);
            // Don't pass transaction ID here:
            // Sampled values are also called charging session meter values,
            // so we relate those to a transaction, not the clock aligned values.
            cp->sendCallAction(MeterValues(this->connectorId, &mv, 1), this->connectorId);
        }
        clock_aligned_meter_values.reset();
    }

    if (charging_session_interval_crossed(&timestamp)) {
        charging_session_meter_values.tick();

        auto to_send = charging_session_meter_values.get(SampledValueContext::SAMPLE_PERIODIC);
        MeterValue mv;
        mv.timestamp = timestamp;
        mv.sampledValue = to_send.sampled_values.get();
        mv.sampledValue_length = to_send.sampled_value_count;
        if (mv.sampledValue_length > 0) {
            log_info("Creating MeterValues.req of connector %d with %lu values (sampled)", this->connectorId, mv.sampledValue_length);
            /* Errata 4.0 3.16: When reporting Meter Values for connectorId 0 (the main energy meter) it is RECOMMENDED NOT to add a TransactionId. */
            cp->sendCallAction(MeterValues(this->connectorId, &mv, 1, (this->connectorId == 0 || !this->transaction_active()) ? OCPP_INTEGER_NOT_PASSED : this->transactionId), this->connectorId);
        }
        charging_session_meter_values.reset();
    }
}

void OcppMeterValueHandler::onStartTransaction(int32_t txnId)
{
    charging_session_meter_values.reset();
    last_charging_session_send = platform_now_ms();
    last_charging_session_send_timestamp = platform_get_system_time(cp->platform_ctx);
    this->transactionId = txnId;
}

bool OcppMeterValueHandler::clock_aligned_interval_crossed(time_t *timestamp) {
    uint32_t interval = getIntConfigUnsigned(ConfigKey::ClockAlignedDataInterval);

    /*
    A value of "0" (numeric zero), by convention, is to be interpreted to mean that no clock-aligned data should be
    transmitted.
    */
    if (interval == 0)
        return false;

    time_t now = platform_get_system_time(cp->platform_ctx);
    struct tm last_send_tm;
    struct tm now_tm;

    gmtime_r(&last_clock_aligned_send_timestamp, &last_send_tm);
    gmtime_r(&now, &now_tm);

    // If !=, we just crossed midnight. This is an interval start in any case, as the intervals are aligned at midnight.
    if (last_send_tm.tm_yday == now_tm.tm_yday){
        uint32_t last_send_seconds_since_midnight = (uint32_t)(last_send_tm.tm_hour * 3600 + last_send_tm.tm_min * 60 + last_send_tm.tm_sec);
        uint32_t now_seconds_since_midnight = (uint32_t)(now_tm.tm_hour * 3600 + now_tm.tm_min * 60 + now_tm.tm_sec);

        uint32_t last_send_interval = last_send_seconds_since_midnight / interval;
        uint32_t now_interval = now_seconds_since_midnight / interval;

        if (now_interval == last_send_interval)
            return false;
    }

    *timestamp = last_clock_aligned_send_timestamp;
    last_clock_aligned_send_timestamp = now;
    return true;
}

bool OcppMeterValueHandler::charging_session_interval_crossed(time_t *timestamp)
{
    uint32_t interval = getIntConfigUnsigned(ConfigKey::MeterValueSampleInterval);
    if (interval == 0)
        return false;

    if (!transaction_active())
        return false;

    if (last_charging_session_send != 0 && !deadline_elapsed(last_charging_session_send + interval * 1000))
        return false;

    *timestamp = last_charging_session_send_timestamp;
    last_charging_session_send = platform_now_ms();
    last_charging_session_send_timestamp = platform_get_system_time(cp->platform_ctx);
    return true;
}
