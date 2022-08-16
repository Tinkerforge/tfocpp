#pragma once

#include <limits>

#include "OcppConnection.h"
#include "OcppConfiguration.h"
#include "OcppMessages.h"
#include "OcppPlatform.h"
#include "OcppTools.h"
#include "OcppDefines.h"
#include "OcppConnector.h"

enum class OcppState {
    PowerOn, // send boot notification, wait for boot notification conf, don't do anything else
    FlushPersistentMessages, // boot notification received, accepted, but we have to send old txn messages first
    Idle, // boot notification received, accepted
    Pending, // boot notification received, pending
    Rejected, // boot notification received, rejected,
    Unavailable,
    Faulted,
    SoftReset,
    HardReset
};

struct IdleInfo {
    IdTagInfo lastSeenTag;
    int32_t lastTagForConnector;
};

struct WaitStartTransConfInfo {
    IdTagInfo lastSeenTag;
    int32_t connectorId;
};

class OcppChargePoint {
public:
    OcppChargePoint() {

    }

    OcppChargePoint (const OcppChargePoint&) = delete;

    void start(const char *websocket_endpoint_url, const char *charge_point_name_percent_encoded);

    void stop() {
        platform_disconnect(platform_ctx);
    }

    void handleTagSeen(int32_t connectorId, const char *tagId);
    void handleStop(int32_t connectorId, StopReason reason);

    void tick_power_on();
    void tick_flush_persistent_messages();
    void tick_idle();

    void tick_soft_reset();
    uint32_t soft_reset_start_ms = 0;

    void tick_hard_reset();
    uint32_t hard_reset_start_ms = 0;

    void tick_clock_aligned_meter_values();


    void tick();

    void onConnect();
    void onDisconnect();
    ChangeAvailabilityResponseStatus onChangeAvailability(ChangeAvailabilityType type);
    void saveAvailability();
    void loadAvailability();


    StatusNotificationStatus last_sent_status = StatusNotificationStatus::NONE;
    StatusNotificationStatus getStatus();
    void sendStatus();
    void forceSendStatus();

    bool sendCallAction(const ICall &call, time_t timestamp = 0);
    void onTimeout(CallAction action, uint32_t messageId);

    CallResponse handleAuthorizeResponse(uint32_t messageId, AuthorizeResponseView conf);
    CallResponse handleBootNotificationResponse(uint32_t messageId, BootNotificationResponseView conf);
    CallResponse handleChangeAvailability(const char *uid, ChangeAvailabilityView req);
    CallResponse handleChangeConfiguration(const char *uid, ChangeConfigurationView req);
    CallResponse handleClearCache(const char *uid, ClearCacheView req);
    CallResponse handleDataTransfer(const char *uid, DataTransferView req);
    CallResponse handleDataTransferResponse(uint32_t messageId, DataTransferResponseView conf);
    CallResponse handleGetConfiguration(const char *uid, GetConfigurationView req);
    CallResponse handleHeartbeatResponse(uint32_t messageId, HeartbeatResponseView conf);
    CallResponse handleMeterValuesResponse(uint32_t messageId, MeterValuesResponseView conf);
    CallResponse handleRemoteStartTransaction(const char *uid, RemoteStartTransactionView req);
    CallResponse handleRemoteStopTransaction(const char *uid, RemoteStopTransactionView req);
    CallResponse handleReset(const char *uid, ResetView req);
    CallResponse handleStartTransactionResponse(uint32_t messageId, StartTransactionResponseView conf);
    CallResponse handleStatusNotificationResponse(uint32_t messageId, StatusNotificationResponseView conf);
    CallResponse handleStopTransactionResponse(uint32_t messageId, StopTransactionResponseView conf);
    CallResponse handleUnlockConnector(const char *uid, UnlockConnectorView req);

    IdleInfo idle_info;
    WaitStartTransConfInfo wstc_info;

    OcppState state = OcppState::PowerOn;

    void *platform_ctx;

    uint32_t last_bn_send_ms = 0;

    OcppConnection connection;

    /*
    Connectors numbering (ConnectorIds) MUST be as follows:
    • ID of the first connector MUST be 1
    • Additional connectors MUST be sequentially numbered (no numbers may be skipped)
    • ConnectorIds MUST never be higher than the total number of connectors of a Charge Point
    • For operations intiated by the Central System, ConnectorId 0 is reserved for addressing the entire Charge
    Point.
    • For operations initiated by the Charge Point (when reporting), ConnectorId 0 is reserved for the Charge
    Point main controller.
    */
    Connector connectors[NUM_CONNECTORS];
};
