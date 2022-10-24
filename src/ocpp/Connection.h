#pragma once

#include <stdint.h>

#include <deque>
#include <memory>

#include "Messages.h"
#include "Platform.h"
#include "Tools.h"

class OcppChargePoint;

class QueueItem {
public:
    CallAction action;
    std::unique_ptr<char[]> buf;
    uint32_t message_id;
    int32_t connector_id;
    size_t len;
    time_t timestamp;

    QueueItem() : action(CallAction::AUTHORIZE), buf(nullptr), message_id(0), connector_id(0), len(0), timestamp(0) {}

    QueueItem(const ICall &call, time_t timestamp, int32_t connector_id) :
        action(call.action),
        buf(nullptr),
        message_id(call.ocppJmessageId),
        connector_id(connector_id),
        len(0),
        timestamp(timestamp)
    {
        auto length = call.measureJson();
        this->buf = heap_alloc_array<char>(length);
        call.serializeJson(this->buf.get(), length);
        this->len = length;
    }

    bool is_valid() {
        return buf != nullptr;
    }
};

class OcppConnection {
public:
    void* start(const char *websocket_endpoint_url, const char *charge_point_name_percent_encoded, OcppChargePoint *ocpp_handle) {
        this->cp = ocpp_handle;
        std::string ws_url;
        ws_url.reserve(strlen(websocket_endpoint_url) + 1 + strlen(charge_point_name_percent_encoded));
        ws_url += websocket_endpoint_url;
        ws_url += '/';
        ws_url += charge_point_name_percent_encoded;

        platform_ctx = platform_init(ws_url.c_str());
        platform_ws_register_receive_callback(platform_ctx, [](char *c, size_t s, void *user_data){((OcppConnection*)user_data)->handleMessage(c, s);}, this);

        return platform_ctx;
    }

    void tick();

    void handleMessage(char *message, size_t message_len);

    void handleCallError(CallErrorCode code, const char *desc, JsonObject details);

    void sendCallError(const char *uid, CallErrorCode code, const char *desc);

    bool sendCallAction(const ICall &call, time_t timestamp = 0, int32_t connectorId = 0);
    bool sendCallResponse(const ICall &call);

    void *platform_ctx;
    OcppChargePoint *cp;

    QueueItem message_in_flight;
    uint32_t message_timeout_deadline;

    uint32_t transaction_message_retry_deadline;
    int transaction_message_attempts = 0;

    std::deque<QueueItem> messages;
    std::deque<QueueItem> status_notifications;
    std::deque<QueueItem> transaction_messages;
};
