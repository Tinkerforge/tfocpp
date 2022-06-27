#pragma once

#include <limits>

#include "OcppConnection.h"
#include "OcppConfiguration.h"
#include "OcppMessages.h"
#include "OcppPlatform.h"
#include "OcppTools.h"

enum class OcppState {
    PowerOn, // send boot notification, wait for boot notification conf, don't do anything else
    Idle, // boot notification received, accepted
    Pending, // boot notification received, pending
    Rejected, // boot notification received, rejected
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

    void tick_power_on();
    void tick_idle();

    void tick();

    void onConnect();
    void onDisconnect();

    bool sendCallAction(CallAction action, const DynamicJsonDocument &doc);

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
};
