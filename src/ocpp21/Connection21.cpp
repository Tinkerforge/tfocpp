#include "Connection21.h"

#include "errno.h"
#include "inttypes.h"
#include "time.h"
#include "TFJson.h"

#include "ChargePoint21.h"
#include <common/Platform.h>
#include <common/Tools.h>

// TODO: make these configurable via the device model (OCPPCommCtrlr).
#define OCPP21_MESSAGE_TIMEOUT_S 30
#define OCPP21_WS_PING_INTERVAL_S 10
#define OCPP21_RECONNECT_INTERVAL_S 10
// TODO: make the depth configurable and persist the queue across reboots.
#define OCPP21_TRANSACTION_QUEUE_DEPTH 32

namespace Ocpp21 {

static void log_payload(const char *prefix, const char *buf, size_t buf_len) {
    log_debug("%s (len %zu) %.*s%s", prefix, buf_len, (int)std::min(buf_len, (size_t)100), buf, buf_len > 96 ? " ..." : "");
}

void Connection::handleMessage(char *message, size_t message_len)
{
    log_payload("Received message", message, message_len);
    DynamicJsonDocument doc{8192};
    DeserializationError error = deserializeJson(doc, message, message_len);
    if (error) {
        log_error("deserializeJson() failed: %s", error.c_str());
        return;
    }
    doc.shrinkToFit();

    if (!doc.is<JsonArray>()) {
        log_error("deserialized JSON is not an array at top level");
        return;
    }

    if (!doc[0].is<int32_t>()) {
        log_error("deserialized JSON array does not start with message type ID");
        return;
    }

    if (!doc[1].is<const char *>()) {
        log_error("deserialized JSON array does not contain unique ID as second member");
        return;
    }

    int32_t messageType = doc[0];
    const char *uniqueID = doc[1];

    if (messageType == (int32_t)OcppRpcMessageType::CALL || messageType == (int32_t)OcppRpcMessageType::SEND) {
        bool is_send = messageType == (int32_t)OcppRpcMessageType::SEND;
        if (doc.size() != 4) {
            log_error("received call with %d members, but expected 4.", (int)doc.size());
            return;
        }

        if (!doc[2].is<const char *>()) {
            log_error("received call with action not being a string.");
            return;
        }

        if (doc[3].isNull() || !doc[3].is<JsonObject>()) {
            log_error("received call with payload being neither an object nor null.");
            return;
        }

        if (cp->state == State::Rejected) {
            // B03.FR.06: while Rejected the Charging Station shall not respond to CSMS initiated messages.
            log_warn("received call while being rejected. Ignoring call.");
            return;
        }

        log_info("Received %s (id %s) action %s", is_send ? "send" : "call", uniqueID, doc[2].as<const char *>());

        CallResponse res = callHandler(uniqueID, doc[2].as<const char *>(), doc[3].as<JsonObject>(), cp);

        // SEND messages are fire and forget. Never respond with an error.
        if (res.result != CallErrorCode::OK && !is_send)
            sendCallError(uniqueID, res.result, res.error_description);

        return;
    }

    if (messageType != (int32_t)OcppRpcMessageType::CALLRESULT
     && messageType != (int32_t)OcppRpcMessageType::CALLERROR
     && messageType != (int32_t)OcppRpcMessageType::CALLRESULTERROR) {
        log_error("received unknown message type %" PRId32, messageType);
        sendCallError(uniqueID, CallErrorCode::MessageTypeNotSupported, "message type number not supported");
        return;
    }

    if (messageType == (int32_t)OcppRpcMessageType::CALLRESULTERROR) {
        // The CSMS rejected one of our call results. We can not do anything
        // about that at this level. Log and continue.
        log_warn("received call result error (id %s): %s", uniqueID, doc[2].is<const char *>() ? doc[2].as<const char *>() : "unknown error code");
        return;
    }

    errno = 0;
    uint64_t uid = strtoull(uniqueID, nullptr, 10);
    if (errno != 0) {
        log_error("received %s with invalid message ID %s.", messageType == 3 ? "call result" : "call error", uniqueID);
        return;
    }

    if (!message_in_flight.is_valid()) {
        log_warn("received %s with message ID %" PRIu64 ", but no call is in flight", messageType == 3 ? "call result" : "call error", uid);
        return;
    }

    if (uid != message_in_flight.message_id) {
        log_error("received %s with message ID %" PRIu64 ". expected was %" PRIu64, messageType == 3 ? "call result" : "call error", uid, message_in_flight.message_id);
        return;
    }

    if (messageType == (int32_t)OcppRpcMessageType::CALLRESULT) {
        CallAction result_to = message_in_flight.action;

        if (doc.size() != 3) {
            log_error("received call result with %d members, but expected 3.", (int)doc.size());
            sendCallResultError(uniqueID, CallErrorCode::RpcFrameworkError, "call result must have 3 members");
            clearInFlight();
            cp->onCallError(result_to, uid);
            return;
        }

        if (doc[2].isNull() || !doc[2].is<JsonObject>()) {
            log_error("received call result with payload being neither an object nor null.");
            sendCallResultError(uniqueID, CallErrorCode::RpcFrameworkError, "call result payload must be an object");
            clearInFlight();
            cp->onCallError(result_to, uid);
            return;
        }

        log_info("Received result for %s (id %" PRIu64 ")", CallActionStrings[(size_t)message_in_flight.action], uid);

        const uint64_t result_message_id = message_in_flight.message_id;
        clearInFlight();
        transaction_message_attempts = 0;

        CallResponse res = callResultHandler(0, result_to, result_message_id, doc[2].as<JsonObject>(), cp);
        // FR.06: an invalid response message is answered with a CALLRESULTERROR.
        if (res.result != CallErrorCode::OK) {
            sendCallResultError(uniqueID, res.result, res.error_description);
            cp->onCallError(result_to, uid);
        }
        return;
    }

    // CALLERROR
    if (doc.size() != 5) {
        log_error("received call error with %d members, but expected 5.", (int)doc.size());
        return;
    }

    log_warn("Received call error (id %" PRIu64 ") %s: %s",
             uid,
             doc[2].is<const char *>() ? doc[2].as<const char *>() : "?",
             doc[3].is<const char *>() ? doc[3].as<const char *>() : "?");

    if (in_flight_is_transaction) {
        ++transaction_message_attempts;
        if (transaction_message_attempts >= (uint32_t)cp->device_model.message_attempts) {
            log_warn("%s (id %" PRIu64 ") failed %u of %d times. Dropping",
                     CallActionStrings[(size_t)message_in_flight.action], message_in_flight.message_id,
                     transaction_message_attempts, cp->device_model.message_attempts);
            transaction_message_attempts = 0;
            cp->onCallError(message_in_flight.action, message_in_flight.message_id);
        } else {
            uint32_t backoff_s = transaction_message_attempts * (uint32_t)cp->device_model.message_attempt_interval_s;
            log_warn("%s (id %" PRIu64 ") failed %u of %d times. Retrying in %u s",
                     CallActionStrings[(size_t)message_in_flight.action], message_in_flight.message_id,
                     transaction_message_attempts, cp->device_model.message_attempts, backoff_s);
            transaction_retry_deadline = set_deadline(backoff_s * 1000);
            transaction_messages.push_front(std::move(message_in_flight));
        }
        clearInFlight();
        return;
    }

    cp->onCallError(message_in_flight.action, message_in_flight.message_id);
    clearInFlight();
}

static size_t buildErrorFrame(TFJsonSerializer &json, OcppRpcMessageType message_type, const char *uid, CallErrorCode code, const char *desc) {
    json.addArray();
    json.addNumber((int32_t)message_type);
    json.addString(uid);
    json.addString(CallErrorCodeStrings[(size_t)code]);
    json.addString(desc);
    json.addObject();
    json.endObject();
    json.endArray();
    return json.end();
}

void Connection::sendErrorFrame(OcppRpcMessageType message_type, const char *uid, CallErrorCode code, const char *desc)
{
    size_t len = 0;
    {
        TFJsonSerializer json{nullptr, 0};
        len = buildErrorFrame(json, message_type, uid, code, desc);
    }
    auto buf = heap_alloc_array<char>(len + 1);
    TFJsonSerializer json{buf.get(), len + 1};
    buildErrorFrame(json, message_type, uid, code, desc);

    QueueItem item;
    item.buf = std::move(buf);
    item.len = len;
    pending_responses.push_back(std::move(item));
}

void Connection::sendCallError(const char *uid, CallErrorCode code, const char *desc)
{
    log_info("Sending error %s (%s) for id %s", CallErrorCodeStrings[(size_t)code], desc, uid);
    sendErrorFrame(OcppRpcMessageType::CALLERROR, uid, code, desc);
}

void Connection::sendCallResultError(const char *uid, CallErrorCode code, const char *desc)
{
    log_info("Sending call result error %s (%s) for id %s", CallErrorCodeStrings[(size_t)code], desc, uid);
    sendErrorFrame(OcppRpcMessageType::CALLRESULTERROR, uid, code, desc);
}

void Connection::clearInFlight()
{
    message_in_flight = QueueItem{};
    in_flight_is_transaction = false;
    message_timeout_deadline = 0;
}

bool Connection::sendCallResponse(const ICall &call)
{
    log_info("Sending response for %s (id %s)", CallActionStrings[(size_t)call.action], call.ocppJcallId);
    pending_responses.emplace_back(call);
    return true;
}

bool Connection::sendCallAction(const ICall &call)
{
    if (!platform_ws_connected(platform_ctx))
        return false;

    if (call.action == CallAction::STATUS_NOTIFICATION) {
        if (status_notifications.size() > 5)
            status_notifications.pop_front();
        status_notifications.emplace_back(call);
    } else {
        if (messages.size() > 5)
            messages.pop_front();
        messages.emplace_back(call);
    }

    return true;
}

bool Connection::sendTransactionCallAction(const ICall &call)
{
    if (transaction_messages.size() >= OCPP21_TRANSACTION_QUEUE_DEPTH) {
        log_warn("Transaction message queue full. Dropping oldest %s", CallActionStrings[(size_t)transaction_messages.front().action]);
        transaction_messages.pop_front();
    }
    transaction_messages.emplace_back(call);
    return true;
}

void Connection::setPongDeadline() {
    this->pong_deadline = set_deadline(1000 * (OCPP21_WS_PING_INTERVAL_S * 3 + OCPP21_WS_PING_INTERVAL_S / 2));
}

void Connection::tick() {
    bool connected = platform_ws_connected(platform_ctx);

    if (!connected && was_connected) {
        cp->onDisconnect();
        connection_state_change_time = platform_get_system_time(platform_ctx);
    } else if (connected && !was_connected) {
        cp->onConnect();
        connection_state_change_time = platform_get_system_time(platform_ctx);

        // Connection establishment counts as successful ping/pong
        last_ping_sent = platform_now_ms();
        this->setPongDeadline();
        // Arm the ping deadline. A plain 0 would never elapse on systems
        // with more than 24.8 days of uptime (uint32 wraparound arithmetic
        // in deadline_elapsed).
        next_ping_deadline = set_deadline(OCPP21_WS_PING_INTERVAL_S * 1000);
        transaction_retry_deadline = set_deadline(0);

        next_reconnect_deadline = 0;
    } else if (!connected && !was_connected) {
        if (next_reconnect_deadline == 0) {
            next_reconnect_deadline = set_deadline(OCPP21_RECONNECT_INTERVAL_S * 1000);
        } else if (deadline_elapsed(next_reconnect_deadline)) {
            platform_reconnect(platform_ctx);
            next_reconnect_deadline = set_deadline(OCPP21_RECONNECT_INTERVAL_S * 1000);
        }
    }

    was_connected = connected;

    if (!connected) {
        status_notifications.clear();
        messages.clear();
        pending_responses.clear();
        // Transaction messages survive the disconnect. An in flight
        // transaction message goes back to the queue front, it is unknown
        // whether the CSMS received it.
        if (message_in_flight.is_valid() && in_flight_is_transaction) {
            transaction_messages.push_front(std::move(message_in_flight));
            message_in_flight = QueueItem{};
            in_flight_is_transaction = false;
            message_timeout_deadline = 0;
        }
        return;
    }

    if (deadline_elapsed(next_ping_deadline)) {
        if (platform_ws_send_ping(platform_ctx)) {
            last_ping_sent = platform_now_ms();
            next_ping_deadline = last_ping_sent + OCPP21_WS_PING_INTERVAL_S * 1000;
        } else {
            log_info("Failed to send ping");
        }
    }

    if (deadline_elapsed(pong_deadline)) {
        log_info("Pong timeout");
        platform_disconnect(platform_ctx);
        return;
    }

    if (!pending_responses.empty()) {
        auto &response = pending_responses.front();
        log_payload("Sending response", response.buf.get(), response.len);
        if (platform_ws_send(platform_ctx, response.buf.get(), response.len))
            pending_responses.pop_front();
        return;
    }

    if (message_in_flight.is_valid()) {
        if (!deadline_elapsed(message_timeout_deadline))
            return;

        if (in_flight_is_transaction) {
            log_info("%s (id %" PRIu64 ") timed out. Retrying in %d s", CallActionStrings[(size_t)message_in_flight.action], message_in_flight.message_id, cp->device_model.message_attempt_interval_s);
            transaction_messages.push_front(std::move(message_in_flight));
            transaction_retry_deadline = set_deadline((uint32_t)cp->device_model.message_attempt_interval_s * 1000);
            in_flight_is_transaction = false;
        } else {
            log_info("%s (id %" PRIu64 ") timed out. Dropping", CallActionStrings[(size_t)message_in_flight.action], message_in_flight.message_id);
            cp->onTimeout(message_in_flight.action, message_in_flight.message_id);
        }
        message_in_flight = QueueItem{};
    }

    bool sending_transaction = false;
    std::deque<QueueItem> *to_pop = nullptr;
    // Transaction events first, they are the authoritative session record
    // and their order relative to the status notifications matters at
    // transaction end (Ended before Available).
    if (!transaction_messages.empty() && deadline_elapsed(transaction_retry_deadline)) {
        to_pop = &transaction_messages;
        sending_transaction = true;
    } else if (!status_notifications.empty()) {
        to_pop = &status_notifications;
    } else if (!messages.empty()) {
        to_pop = &messages;
    } else
        return;

    {
        QueueItem *to_send = &to_pop->front();

        auto new_deadline = set_deadline(1000 * OCPP21_MESSAGE_TIMEOUT_S);

        log_payload("Sending request", to_send->buf.get(), to_send->len);
        if (!platform_ws_send(platform_ctx, to_send->buf.get(), to_send->len)) {
            log_info("Send failed");
            return;
        }

        this->message_timeout_deadline = new_deadline;
    }

    message_in_flight = std::move(to_pop->front());
    to_pop->pop_front();
    in_flight_is_transaction = sending_transaction;
}

QueueItem::QueueItem(const ICall &call) :
        action(call.action),
        buf(nullptr),
        message_id(call.ocppJmessageId),
        len(0) {
    auto length = call.measureJson();
    this->buf = heap_alloc_array<char>(length + 1);
    call.serializeJson(this->buf.get(), length + 1);
    this->len = length;
}

bool QueueItem::is_valid()
{
    return buf != nullptr;
}

static std::unique_ptr<BasicAuthCredentials[]> build_credentials(const char *user, const char *pass)
{
    auto creds = heap_alloc_array<BasicAuthCredentials>(1);

    auto user_len = strlen(user) + 1;
    creds[0].user = heap_alloc_array<char>(user_len);
    memcpy(creds[0].user.get(), user, user_len);

    auto pass_len = strlen(pass);
    creds[0].pass = heap_alloc_array<uint8_t>(pass_len);
    memcpy(creds[0].pass.get(), pass, pass_len);
    creds[0].pass_length = pass_len;

    return creds;
}

void *Connection::start(const char *websocket_endpoint_url, const char *charge_point_name_percent_encoded, const char *basic_auth_pass, const PlatformTlsConfig *tls, ChargePoint *ocpp_handle) {
    this->cp = ocpp_handle;
    this->charge_point_name = charge_point_name_percent_encoded;

    // Message ids restarting at 0 after a reboot collide with the previous
    // session. Some CSMS deduplicate calls by message id and answer with
    // the cached response of the previous session. Seed the counter once
    // per boot.
    if (next_call_id == 0)
        next_call_id = (uint64_t)time(nullptr) * 1000;

    std::string ws_url;
    ws_url.reserve(strlen(websocket_endpoint_url) + 1 + strlen(charge_point_name_percent_encoded));
    ws_url += websocket_endpoint_url;
    ws_url += '/';
    ws_url += charge_point_name_percent_encoded;

    size_t cred_used_count = 0;
    if (basic_auth_pass != nullptr && basic_auth_pass[0] != '\0') {
        this->basic_auth_credentials = build_credentials(charge_point_name_percent_encoded, basic_auth_pass);
        cred_used_count = 1;
    }

    platform_ctx = platform_init(ws_url.c_str(), "ocpp2.1", this->basic_auth_credentials.get(), cred_used_count, tls);
    if (platform_ctx == nullptr)
        return nullptr;

    platform_ws_register_receive_callback(platform_ctx, [](char *c, size_t s, void *user_data){((Connection*)user_data)->handleMessage(c, s);}, this);
    platform_ws_register_pong_callback(platform_ctx, [](void *user_data){((Connection*)user_data)->setPongDeadline();}, this);

    return platform_ctx;
}

void Connection::updateBasicAuthPassword(const char *basic_auth_pass) {
    size_t cred_used_count = 0;
    if (basic_auth_pass != nullptr && basic_auth_pass[0] != '\0') {
        this->basic_auth_credentials = build_credentials(charge_point_name.c_str(), basic_auth_pass);
        cred_used_count = 1;
    } else {
        this->basic_auth_credentials = nullptr;
    }

    platform_update_credentials(platform_ctx, this->basic_auth_credentials.get(), cred_used_count);
    platform_reconnect(platform_ctx);
}

void Connection::updateEndpoint(const char *websocket_endpoint_url, const char *basic_auth_pass, const PlatformTlsConfig *tls) {
    std::string ws_url;
    ws_url.reserve(strlen(websocket_endpoint_url) + 1 + charge_point_name.size());
    ws_url += websocket_endpoint_url;
    ws_url += '/';
    ws_url += charge_point_name;

    size_t cred_used_count = 0;
    if (basic_auth_pass != nullptr && basic_auth_pass[0] != '\0') {
        this->basic_auth_credentials = build_credentials(charge_point_name.c_str(), basic_auth_pass);
        cred_used_count = 1;
    } else {
        this->basic_auth_credentials = nullptr;
    }

    platform_update_tls(platform_ctx, tls);
    platform_update_credentials(platform_ctx, this->basic_auth_credentials.get(), cred_used_count);
    platform_update_url(platform_ctx, ws_url.c_str());
    platform_reconnect(platform_ctx);
}

void Connection::stop() {
    platform_ws_register_pong_callback(platform_ctx, nullptr, nullptr);
    platform_ws_register_receive_callback(platform_ctx, nullptr, nullptr);

    platform_disconnect(platform_ctx);
    platform_destroy(platform_ctx);
}

} // namespace Ocpp21
