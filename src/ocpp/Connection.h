#pragma once

#include <stdint.h>

#include <deque>
#include <memory>

#include "Messages.h"
#include "Types.h"

class OcppChargePoint;

class QueueItem {
public:
    CallAction action;
    std::unique_ptr<char[]> buf;
    uint64_t message_id;
    int32_t connector_id;
    size_t len;

    QueueItem() : action(CallAction::AUTHORIZE), buf(nullptr), message_id(0), connector_id(0), len(0) {}

    QueueItem(const ICall &call, int32_t connector_id);

    bool is_valid();
};


enum class BasicAuthPassType {
    // No credentials passed.
    NONE,
    // The given basic auth pass is 40 hex chars to be interpreted and send as 20 bytes as per the OCPPJ spec.
    HEX_CHARS,
    // The given basic auth pass should not be interpreted but sent as is.
    TEXT,
    // Interpret as hex chars first and if the server does not accept us, retry as text.
    TRY_BOTH
};

class OcppConnection {
public:
    void* start(const char *websocket_endpoint_url, const char *charge_point_name_percent_encoded, const char *basic_auth_user, const uint8_t *basic_auth_pass, size_t basic_auth_pass_length, BasicAuthPassType basic_auth_pass_type, OcppChargePoint *ocpp_handle);

    void tick();

    void handleMessage(char *message, size_t message_len);

    void handleCallError(uint64_t uid, CallErrorCode code, const char *desc, JsonObject details);

    void sendCallError(const char *uid, CallErrorCode code, const char *desc);

    bool sendCallAction(const ICall &call, int32_t connectorId = 0);
    bool sendCallResponse(const ICall &call);

    void setPongDeadline();

    void *platform_ctx;
    OcppChargePoint *cp;

    QueueItem message_in_flight;
    uint32_t message_timeout_deadline = 0;

    time_t connection_state_change_time = 0;
    uint32_t next_ping_deadline = 0;
    uint32_t last_ping_sent = 0;
    uint32_t pong_deadline = 0;
    uint32_t next_reconnect_deadline = 0;
    // FIXME: this is ugly as hell. Store retry counter and deadline in queue item!
    uint32_t transaction_message_retry_deadline = 0;
    uint32_t transaction_message_attempts = 0;

    std::deque<QueueItem> messages;
    std::deque<QueueItem> status_notifications;
    std::deque<QueueItem> transaction_messages;
    // As there can only be one call in flight, we don't need a queue here.
    QueueItem next_response;

    std::unique_ptr<BasicAuthCredentials[]> basic_auth_credentials;
};
