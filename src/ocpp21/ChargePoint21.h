#pragma once

#include <time.h>

#include "Connection21.h"
#include "DeviceModel21.h"
#include "Messages21.h"
#include "Types21.h"

namespace Ocpp21 {

enum class OcppState21 {
    PowerOn,  // boot notification not yet accepted
    Pending,  // boot notification answered with Pending
    Rejected, // boot notification answered with Rejected
    Idle,     // boot notification accepted
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

    // Received call results
    CallResponse handleBootNotificationResponse(int32_t connectorId, BootNotificationResponseView conf);
    CallResponse handleHeartbeatResponse(int32_t connectorId, HeartbeatResponseView conf);
    CallResponse handleStatusNotificationResponse(int32_t connectorId, StatusNotificationResponseView conf);

    // Received calls
    CallResponse handleGetVariables(const char *uid, GetVariablesView req);
    CallResponse handleSetVariables(const char *uid, SetVariablesView req);
    CallResponse handleGetBaseReport(const char *uid, GetBaseReportView req);
    CallResponse handleTriggerMessage(const char *uid, TriggerMessageView req);
    CallResponse handleReset(const char *uid, ResetView req);

    OcppState21 state = OcppState21::PowerOn;

    Connection21 connection;
    DeviceModel21 device_model;

private:
    void sendBootNotification();
    void sendStatusNotifications();

    uint32_t boot_retry_deadline = 0;
    // Interval requested by the CSMS in a Pending or Rejected boot response. 0 = use default.
    int32_t boot_retry_interval_s = 0;
    bool boot_notification_in_flight = false;

    uint32_t next_heartbeat_deadline = 0;

    bool status_notifications_pending = false;

    bool trigger_boot_notification = false;
    bool trigger_heartbeat = false;
    bool trigger_status_notification = false;
};

} // namespace Ocpp21
