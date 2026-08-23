#pragma once

#include <time.h>

#include "Connection21.h"
#include "DeviceModel21.h"
#include "Messages21.h"
#include "Platform21.h"
#include "Types21.h"

namespace Ocpp21 {

enum class OcppState21 {
    PowerOn,  // boot notification not yet accepted
    Pending,  // boot notification answered with Pending
    Rejected, // boot notification answered with Rejected
    Idle,     // boot notification accepted
};

#define OCPP21_ID_TOKEN_LEN 64
#define OCPP21_ID_TOKEN_TYPE_LEN 20
#define OCPP21_TRANSACTION_ID_LEN 36

// Per EVSE connector and transaction state. One connector per EVSE,
// TxStartPoint and TxStopPoint fixed to PowerPathClosed (simplified to
// authorized and cable plugged).
struct EvseTracker21 {
    EvseState21 last_state = EvseState21::NotConnected;
    StatusNotificationConnectorStatus last_sent_status = StatusNotificationConnectorStatus::NONE;

    // Authorization waiting for or attached to a transaction.
    bool authorized = false;
    char id_token[OCPP21_ID_TOKEN_LEN + 1] = {};
    char id_token_type[OCPP21_ID_TOKEN_TYPE_LEN + 1] = {};
    bool remote_start = false;
    int32_t remote_start_id = 0;
    // Trigger reason for the Started event, set when the authorization is granted.
    TransactionEventTriggerReason start_trigger = TransactionEventTriggerReason::NONE;
    // EVConnectionTimeOut while authorized but no cable plugged.
    uint32_t ev_connect_deadline = 0;

    // Active transaction.
    bool transaction_active = false;
    char transaction_id[OCPP21_TRANSACTION_ID_LEN + 1] = {};
    int32_t seq_no = 0;
    TransactionEventTransactionInfoChargingState charging_state = TransactionEventTransactionInfoChargingState::NONE;
    uint32_t next_sampled_value_deadline = 0;
};

class ChargePoint21 {
public:
    ChargePoint21() {}

    ChargePoint21(const ChargePoint21&) = delete;
    ChargePoint21 &operator=(const ChargePoint21&) = delete;

    bool start(const char *websocket_endpoint_url, const char *charge_point_name, const char *basic_auth_pass);
    void stop();
    void tick();

    // Connection events
    void onConnect();
    void onDisconnect();
    void onTimeout(CallAction action, uint64_t messageId);
    void onCallError(CallAction action, uint64_t messageId);

    // Platform events
    void onTagSeen(int32_t evse_id, const char *tag_id);

    // Received call results
    CallResponse handleBootNotificationResponse(int32_t connectorId, BootNotificationResponseView conf);
    CallResponse handleHeartbeatResponse(int32_t connectorId, HeartbeatResponseView conf);
    CallResponse handleStatusNotificationResponse(int32_t connectorId, StatusNotificationResponseView conf);
    CallResponse handleAuthorizeResponse(int32_t connectorId, AuthorizeResponseView conf);
    CallResponse handleTransactionEventResponse(int32_t connectorId, TransactionEventResponseView conf);
    CallResponse handleMeterValuesResponse(int32_t connectorId, MeterValuesResponseView conf);

    // Received calls
    CallResponse handleGetVariables(const char *uid, GetVariablesView req);
    CallResponse handleSetVariables(const char *uid, SetVariablesView req);
    CallResponse handleGetBaseReport(const char *uid, GetBaseReportView req);
    CallResponse handleTriggerMessage(const char *uid, TriggerMessageView req);
    CallResponse handleReset(const char *uid, ResetView req);
    CallResponse handleRequestStartTransaction(const char *uid, RequestStartTransactionView req);
    CallResponse handleRequestStopTransaction(const char *uid, RequestStopTransactionView req);

    OcppState21 state = OcppState21::PowerOn;

    Connection21 connection;
    DeviceModel21 device_model;

private:
    void sendBootNotification();
    void sendStatusNotifications();
    void tickEvses();
    void startTransaction(int32_t evse_id, TransactionEventTriggerReason trigger);
    void stopTransaction(int32_t evse_id, TransactionEventTriggerReason trigger, TransactionEventTransactionInfoStoppedReason reason, bool include_token);
    void sendTransactionUpdated(int32_t evse_id, TransactionEventTriggerReason trigger, bool with_meter_value);

    uint32_t boot_retry_deadline = 0;
    // Interval requested by the CSMS in a Pending or Rejected boot response. 0 = use default.
    int32_t boot_retry_interval_s = 0;
    bool boot_notification_in_flight = false;

    uint32_t next_heartbeat_deadline = 0;

    bool status_notifications_pending = false;

    bool trigger_boot_notification = false;
    bool trigger_heartbeat = false;
    bool trigger_status_notification = false;

    EvseTracker21 evses[1];

    // A tag swipe reported by the platform, handled in the next tick.
    bool tag_pending = false;
    int32_t tag_evse_id = 0;
    char pending_tag[OCPP21_ID_TOKEN_LEN + 1] = {};

    // Only one Authorize is in flight at a time.
    bool authorize_in_flight = false;
    int32_t authorize_evse_id = 0;
    char authorize_token[OCPP21_ID_TOKEN_LEN + 1] = {};
};

} // namespace Ocpp21
