#pragma once

#include "MeterValueAccumulator.h"

class OcppChargePoint;

struct OcppMeterValueHandler {
    void init(int32_t connId, OcppChargePoint *chargePoint);

    void tick();
    void onStartTransaction(int32_t txnId);

    void onStopTransaction() {
        // TODO: add stoptxn meter values here.
        // Probably return those so that the connector (that call this method)
        // can add the values to the StopTransaction.req

        charging_session_meter_values.reset();
        this->transactionId = INT32_MAX;
    }

    bool clock_aligned_interval_crossed(time_t *timestamp);
    bool charging_session_interval_crossed(time_t *timestamp);
    bool transaction_active() { return transactionId != INT32_MAX;}

    int connectorId;
    OcppChargePoint *cp;

    MeterValueAccumulator clock_aligned_meter_values;
    MeterValueAccumulator charging_session_meter_values;

    uint32_t last_run = 0;
    time_t last_clock_aligned_send_timestamp = 0;
    uint32_t last_charging_session_send = 0;
    time_t last_charging_session_send_timestamp = 0;

    int32_t transactionId = INT32_MAX;
};
