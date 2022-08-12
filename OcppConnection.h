#pragma once

#include <stdint.h>

#include <deque>
#include <memory>

#include "OcppMessages.h"
#include "OcppPlatform.h"

class OcppChargePoint;

class QueueItem {
public:
    CallAction action;
    std::unique_ptr<char[]> buf;
    uint32_t message_id;
    size_t len;

    QueueItem() : action(CallAction::AUTHORIZE), buf(nullptr), message_id(0), len(0) {}

    QueueItem(CallAction action, const DynamicJsonDocument &doc) :
        action(action),
        buf(nullptr),
        message_id(doc[1].as<uint32_t>()),
        len(0),
    {
        auto length = measureJson(doc);

        this->buf.reset(new char[length]);
        serializeJson(doc, this->buf.get(), length);
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

    void sendCallError(const char *uid, CallErrorCode code, const char *desc, JsonObject details);

    bool sendCallAction(CallAction action, const DynamicJsonDocument &doc);
    bool sendCallResponse(const DynamicJsonDocument &doc);

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
