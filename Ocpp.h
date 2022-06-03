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
    Rejected, // boot notification received, rejected
    WaitingForConnectorSelection, // autorization accepted, will start transaction when connector is selected
    WaitingForStartTransactionConf // connector selected, start transaction sent. will start charging when starttransaction.conf is received
};


struct IdTagInfo {
    char tagId[21] = {0};
    char parentTagId[21] = {0};
    ResponseIdTagInfoEntriesStatus status = ResponseIdTagInfoEntriesStatus::INVALID;
    time_t expiryDate = 0;

    void updateTagId(const char *newTagId) {
        memset(tagId, 0, ARRAY_SIZE(tagId));
        strncpy(tagId, newTagId, ARRAY_SIZE(tagId) - 1);
    }

    void updateFromIdTagInfo(StopTransactionResponseIdTagInfoEntriesView view) {
        memset(parentTagId, 0, ARRAY_SIZE(parentTagId));
        if (view.parentIdTag().is_set())
            strncpy(parentTagId, view.parentIdTag().get(), ARRAY_SIZE(parentTagId) - 1);

        expiryDate = view.expiryDate().is_set() ? view.expiryDate().get() : 0;
        status = view.status();
    }

    void updateFromIdTagInfo(StartTransactionResponseIdTagInfoEntriesView view) {
        memset(parentTagId, 0, ARRAY_SIZE(parentTagId));
        if (view.parentIdTag().is_set())
            strncpy(parentTagId, view.parentIdTag().get(), ARRAY_SIZE(parentTagId) - 1);

        expiryDate = view.expiryDate().is_set() ? view.expiryDate().get() : 0;
        status = view.status();
    }

    void updateFromIdTagInfo(AuthorizeResponseIdTagInfoEntriesView view) {
        memset(parentTagId, 0, ARRAY_SIZE(parentTagId));
        if (view.parentIdTag().is_set())
            strncpy(parentTagId, view.parentIdTag().get(), ARRAY_SIZE(parentTagId) - 1);

        expiryDate = view.expiryDate().is_set() ? view.expiryDate().get() : 0;
        status = view.status();
    }
};

struct IdleInfo {
    IdTagInfo lastSeenTag;
};

struct WaitStartTransConfInfo {
    IdTagInfo lastSeenTag;
    int32_t connectorId;
};

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
        platform_ws_register_receive_callback(platform_ctx, [](char *c, size_t s, void *user_data){((Ocpp*)user_data)->handleMessage(c, s);}, this);
        platform_register_tag_seen_callback(platform_ctx, [](const char *tagId, void *user_data){((Ocpp*)user_data)->handleTagSeen(tagId);}, this);
        platform_register_select_connector_callback(platform_ctx, [](int32_t connectorId, void *user_data){((Ocpp*)user_data)->handleConnectorSelection(connectorId);}, this);
    }

    void stop() {
        platform_disconnect(platform_ctx);
    }

    void handleTagSeen(const char *tagId);
    void handleConnectorSelection(int32_t connectorId);

    void tick_power_on();
    void tick_idle();

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

    IdleInfo idle_info;
    WaitStartTransConfInfo wstc_info;

private:
    void *platform_ctx;

    uint32_t last_bn_send_ms = 0;

    uint32_t last_call_message_id = 0;
    CallAction last_call_action;
};
