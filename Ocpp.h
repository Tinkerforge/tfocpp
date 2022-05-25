#pragma once

#include <limits>

#include "OcppConfiguration.h"
#include "OcppMessages.h"
#include "OcppPlatform.h"
#include "OcppTools.h"

enum class OcppState {
    PowerOn, // send boot notification, wait for boot notification conf, don't do anything else
    Idle, // boot notification received, accepted
    Pending, // boot notification received, pending
    Rejected // boot notification received, rejected
};

#define DEFAULT_BOOT_NOTIFICATION_RESEND_INTERVAL_MS 60000

class Ocpp {
public:
    Ocpp() {

    }

    void start(const char *websocket_endpoint_url, const char *charge_point_name_percent_encoded) {
        std::string ws_url;
        ws_url.reserve(strlen(websocket_endpoint_url) + 1 + strlen(charge_point_name_percent_encoded));
        ws_url += websocket_endpoint_url;
        ws_url += '/';
        ws_url += charge_point_name_percent_encoded;

        platform_ctx = platform_init(ws_url.c_str());
        platform_ws_register_receive_callback(platform_ctx, [this](char *c, size_t s){handleMessage(c, s);});
    }

    void tick_power_on();

    void tick();

    void handleMessage(char *message, size_t message_len);

    void sendCallError(const char *uid, CallErrorCode code, const char *desc, JsonObject details);

    CallResponse handleAuthorizeResponse(AuthorizeResponseView conf);
    CallResponse handleBootNotificationResponse(BootNotificationResponseView conf);
    CallResponse handleChangeAvailability(const char *uid, ChangeAvailabilityView req);
    CallResponse handleChangeConfiguration(const char *uid, ChangeConfigurationView req);
    CallResponse handleClearCache(const char *uid, ClearCacheView req);
    CallResponse handleDataTransfer(const char *uid, DataTransferView req);
    CallResponse handleDataTransferResponse(DataTransferResponseView conf);
    CallResponse handleGetConfiguration(const char *uid, GetConfigurationView req);
    CallResponse handleHeartbeatResponse(HeartbeatResponseView conf);
    CallResponse handleMeterValuesResponse(MeterValuesResponseView conf);
    CallResponse handleRemoteStartTransaction(const char *uid, RemoteStartTransactionView req);
    CallResponse handleRemoteStopTransaction(const char *uid, RemoteStopTransactionView req);
    CallResponse handleReset(const char *uid, ResetView req);
    CallResponse handleStartTransactionResponse(StartTransactionResponseView conf);
    CallResponse handleStatusNotificationResponse(StatusNotificationResponseView conf);
    CallResponse handleStopTransactionResponse(StopTransactionResponseView conf);
    CallResponse handleUnlockConnector(const char *uid, UnlockConnectorView req);

    void handleCallError(CallErrorCode code, const char *desc, JsonObject details);


    OcppState state = OcppState::PowerOn;

private:
    void *platform_ctx;

    uint32_t last_bn_send_ms = 0;

    uint32_t last_call_message_id = 0;
    CallAction last_call_action;
};
