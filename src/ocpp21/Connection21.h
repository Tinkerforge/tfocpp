#pragma once

#include <stdint.h>

#include <deque>
#include <memory>

#include "Messages21.h"
#include "Types21.h"

namespace Ocpp21 {

class ChargePoint21;

class QueueItem21 {
public:
    CallAction action;
    std::unique_ptr<char[]> buf;
    uint64_t message_id;
    size_t len;

    QueueItem21() : action(CallAction::BOOT_NOTIFICATION), buf(nullptr), message_id(0), len(0) {}

    QueueItem21(const ICall &call);

    bool is_valid();
};

class Connection21 {
public:
    void *start(const char *websocket_endpoint_url, const char *charge_point_name_percent_encoded, const char *basic_auth_pass, ChargePoint21 *ocpp_handle);

    void stop();

    void tick();

    void handleMessage(char *message, size_t message_len);

    void sendCallError(const char *uid, CallErrorCode code, const char *desc);

    bool sendCallAction(const ICall &call);
    bool sendCallResponse(const ICall &call);

    void setPongDeadline();

    void *platform_ctx = nullptr;
    ChargePoint21 *cp = nullptr;

    QueueItem21 message_in_flight;
    uint32_t message_timeout_deadline = 0;

    time_t connection_state_change_time = 0;
    uint32_t next_ping_deadline = 0;
    uint32_t last_ping_sent = 0;
    uint32_t pong_deadline = 0;
    uint32_t next_reconnect_deadline = 0;

    std::deque<QueueItem21> messages;
    std::deque<QueueItem21> status_notifications;
    // As there can only be one call in flight, we don't need a queue here.
    QueueItem21 next_response;

    std::unique_ptr<BasicAuthCredentials[]> basic_auth_credentials;

    bool was_connected = false;
};

} // namespace Ocpp21
