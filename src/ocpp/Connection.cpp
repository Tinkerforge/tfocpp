#include "Connection.h"

#include "errno.h"
#include "inttypes.h"
#include "TFJson.h"

#include "ChargePoint.h"
#include "Persistency.h"
#include "Types.h"
#include "Platform.h"

static bool is_transaction_related(CallAction action) {
     // TODO: only "periodic or clock-aligned MeterValues.req messages" are transaction related. Are those all MeterValues messages?
    return action == CallAction::START_TRANSACTION
        || action == CallAction::STOP_TRANSACTION
        || action == CallAction::METER_VALUES;
}

static void log_payload(const char *prefix, const char *buf, size_t buf_len) {
    log_debug("%s (len %zu) %.*s%s", prefix, buf_len, (int)std::min(buf_len, (size_t)100), buf, buf_len > 96 ? " ..." : "");
}

void OcppConnection::handleMessage(char *message, size_t message_len)
{
    (void) message_len;
    log_payload("Received message", message, message_len);
    DynamicJsonDocument doc{4096};
    // TODO: we should use
    // https://arduinojson.org/v6/how-to/deserialize-a-very-large-document/#deserialization-in-chunks
    // to parse each member in the top level array by its own.
    // This would allow us to send CallErrors back to the central if
    // we receive a message that for example can not be completely
    // parsed as JSON.
    DeserializationError error = deserializeJson(doc, message);
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
        log_error("deserialized JSON array does not contain unique ID as second member ");
        return;
    }

    int32_t messageType = doc[0];
    const char *uniqueID = doc[1];

    if (messageType == (int32_t)OcppRpcMessageType::CALL) {
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

        if (cp->state == OcppState::Rejected) {
            // "While Rejected, the Charge Point SHALL NOT respond to any Central System initiated message. the Central System SHOULD NOT initiate any."
            log_warn("received call while being rejected. Ignoring call.");
            return;
        }

        log_info("Received call (id %s)", uniqueID);

        CallResponse res = callHandler(uniqueID, doc[2].as<const char *>(), doc[3].as<JsonObject>(), cp);

        if (res.result != CallErrorCode::OK)
            sendCallError(uniqueID, res.result, res.error_description);

        return;
    }

    if (messageType != (int32_t)OcppRpcMessageType::CALLRESULT && messageType != (int32_t)OcppRpcMessageType::CALLERROR) {
        log_error("received unknown message type %d", messageType);
        return;
    }

    errno = 0;
    uint64_t uid = strtoull(uniqueID, nullptr, 10);
    if (errno != 0) {
        log_error("received %s with invalid message ID %s. ", messageType == 3 ? "call result" : "call error", uniqueID);
        return;
    }

    if (!message_in_flight.is_valid()) {
        log_warn("received %s with message ID %" PRIu64 ", but no call is in flight", messageType == 3 ? "call result" : "call error", uid);
        return;
    }

    uint64_t last_call_message_id = message_in_flight.message_id;

    if (uid != last_call_message_id) {
        log_error("received %s with message ID %" PRIu64 ". expected was %" PRIu64, messageType == 3 ? "call result" : "call error", uid, last_call_message_id);
        return;
    }

    if (messageType == (int32_t)OcppRpcMessageType::CALLRESULT) {
        if (doc.size() != 3) {
            log_error("received call result with %d members, but expected 3.", (int)doc.size());
            return;
        }

        if (doc[2].isNull() || !doc[2].is<JsonObject>()) {
            log_error("received call result with payload being neither an object nor null.");
            return;
        }

        if (is_transaction_related(message_in_flight.action))
            onTxnMsgResponseReceived(message_in_flight.message_id);

        log_info("Received result for %s (id %" PRIu64 ")", CallActionStrings[(size_t) message_in_flight.action], uid);

        CallResponse res = callResultHandler(message_in_flight.connector_id, message_in_flight.action, doc[2].as<JsonObject>(), cp);
        message_in_flight = QueueItem{};
        transaction_message_retry_deadline = 0;
        transaction_message_attempts = 0;
        message_timeout_deadline = 0;

        (void)res;
        /*if (res.result != CallErrorCode::OK)
            sendCallError(uniqueID, res.result, res.error_description);*/
        return;
    }

    if (messageType == (int32_t)OcppRpcMessageType::CALLERROR) {
        if (doc.size() != 5) {
            log_error("received call error with %d members, but expected 5.", (int)doc.size());
            return;
        }

        if (!doc[2].is<const char *>()) {
            log_error("received call error with error code not being a string!.");
            return;
        }

        if (!doc[3].is<const char *>()) {
            log_error("received call error with error description not being a string!.");
            return;
        }

        if (!doc[4].is<JsonObject>()) {
            log_error("received call error with error details not being an object!.");
            return;
        }

        size_t cec = (size_t) CallErrorCode::GenericError;
        if (!lookup_key(&cec, doc[2], CallErrorCodeStrings, (size_t)CallErrorCode::OK, CallErrorCodeStringAliases, CallErrorCodeStringAliasIndices, CallErrorCodeStringAliasLength)) {
            log_error("received call error with unknown error code '%s'! Replacing with GenericError.", doc[2].as<const char *>());
        }

        handleCallError(uid, (CallErrorCode)cec, doc[3], doc[4]);
        return;
    }
}

void OcppConnection::handleCallError(uint64_t uid, CallErrorCode code, const char *desc, JsonObject details)
{
    std::string details_string;
    serializeJsonPretty(details, details_string);
    log_warn("Received call error (id %" PRIu64 ") %s %s:\n %s\n-----end of received call error-----", uid, CallErrorCodeStrings[(size_t)code], desc, details_string.c_str());

    if (!is_transaction_related(message_in_flight.action)) {
        cp->onTimeout(message_in_flight.action, message_in_flight.message_id, message_in_flight.connector_id);
        message_in_flight = QueueItem{};
        return;
    }

    auto max_attempts = getIntConfigUnsigned(ConfigKey::TransactionMessageAttempts);
    if (transaction_message_attempts >= (max_attempts - 1)) {
        log_warn("Transaction related message (id %" PRIu64 ") resulted in a call error %d of max %d times. Dropping.", message_in_flight.message_id, transaction_message_attempts + 1, max_attempts);
        cp->onTimeout(message_in_flight.action, message_in_flight.message_id, message_in_flight.connector_id);
        message_in_flight = QueueItem{};
        transaction_message_attempts = 0;
        return;
    } else {
        transaction_messages.push_front(std::move(message_in_flight));
        ++transaction_message_attempts;

        auto new_timeout_s = transaction_message_attempts * getIntConfigUnsigned(ConfigKey::TransactionMessageRetryInterval);
        transaction_message_retry_deadline = set_deadline(new_timeout_s * 1000);

        log_warn("Transaction related message (id %" PRIu64 ") resulted in a call error %d of max %d times. Waiting %d seconds before resending.", message_in_flight.message_id, transaction_message_attempts, max_attempts, new_timeout_s);
    }
}

static size_t buildCallError(TFJsonSerializer &json, const char *uid, CallErrorCode code, const char *desc) {
    json.addArray();
    json.addNumber((int32_t)OcppRpcMessageType::CALLERROR);
    json.addString(uid);
    json.addString(CallErrorCodeStrings[(size_t)code]);
    json.addString(desc);
    json.addObject();
    json.endObject();
    json.endArray();
    return json.end();
}

void OcppConnection::sendCallError(const char *uid, CallErrorCode code, const char *desc)
{
    log_info("Sending error %s (%s) for id %s", CallErrorCodeStrings[(size_t) code], desc, uid);

    size_t len = 0;
    {
        TFJsonSerializer json{nullptr, 0};
        len = buildCallError(json, uid, code, desc);
    }
    // TFJson will write a null terminator if the buffer is big enough.
    auto buf = heap_alloc_array<char>(len + 1);
    TFJsonSerializer json{buf.get(), len + 1};
    buildCallError(json, uid, code, desc);

    next_response.buf = std::move(buf);
    next_response.len = len;
}

bool OcppConnection::sendCallResponse(const ICall &call)
{
    log_info("Sending response for %s (id %s)", CallActionStrings[(size_t) call.action], call.ocppJcallId);
    next_response = QueueItem{call, 0};
    return true;
}

bool OcppConnection::sendCallAction(const ICall &call, int32_t connectorId)
{
    if (is_transaction_related(call.action)) {
        // Meter values are transaction releated messages. However the amount of energy charged in a transaction
        // can be calculated with only the Start and StopTransaction.reqs. If we get a new transaction related message
        // and the queue is full, we drop the oldest meter values message. The queue should then always have enough room, because
        // only Start and StopTxn messages will be emplaced permanently in the txn_msg queue.
        // To create a StartTxn message the connection must work, because we don't implement any way of authenticating offline.
        // So only if the Authorize message gets through, we can create a StartTxn message.
        // The worst case now is that we lose the connection directly after the Authorize message, and the transaction continues.
        // We then have enqueued at most two non-meter-value messages, one StartTxn and one StopTxn.
        if (transaction_messages.size() >= OCPP_TRANSACTION_RELATED_MESSAGE_QUEUE_SIZE) {
            for(size_t i = 0; i < transaction_messages.size(); ++i) {
                if (transaction_messages[i].action == CallAction::METER_VALUES) {
                    transaction_messages.erase(transaction_messages.begin() + i);
                    break;
                }
            }
        }
        transaction_messages.emplace_back(call, connectorId);
        return true;
    }

    // drop not transaction related messages and status notifications:
    // "The Charge Point MAY send a StatusNotification.req PDU to report an error that occurred while the Charge Point was offline."
    if (!platform_ws_connected(platform_ctx))
        return false;

    if (call.action == CallAction::STATUS_NOTIFICATION) {
        if (status_notifications.size() > 5)
            status_notifications.pop_front();
        status_notifications.emplace_back(call, connectorId);
    }
    else {
        if (messages.size() > 5)
            messages.pop_front();
        messages.emplace_back(call, connectorId);
    }

    return true;
}

void OcppConnection::tick() {
    static bool was_connected = false;
    bool connected = platform_ws_connected(platform_ctx);

    if (!connected && was_connected) {
        cp->onDisconnect();
        connection_state_change_time = platform_get_system_time(platform_ctx);
    } else if (connected && !was_connected) {
        cp->onConnect();
        connection_state_change_time = platform_get_system_time(platform_ctx);

        // Connection establishment counts as successful ping/pong
        last_ping_sent = platform_now_ms();
        pong_deadline = set_deadline(OCPP_WEBSOCKET_PING_PONG_TIMEOUT * 1000);

        // Stop reconnects
        next_reconnect_deadline = 0;
    } else if (!connected && !was_connected) {
        if (next_reconnect_deadline == 0) {
            next_reconnect_deadline = set_deadline(OCPP_RECONNECT_WEBSOCKET_INTERVAL_S * 1000);
        } else if (deadline_elapsed(next_reconnect_deadline)) {
            platform_reconnect(platform_ctx);
            next_reconnect_deadline = set_deadline(OCPP_RECONNECT_WEBSOCKET_INTERVAL_S * 1000);
        }
    }

#ifdef OCPP_STATE_CALLBACKS
    platform_update_connection_state(
        message_in_flight.action,
        message_in_flight.message_id,
        message_in_flight.len,
        message_timeout_deadline,
        transaction_message_retry_deadline,
        (uint8_t)messages.size(),
        (uint8_t)status_notifications.size(),
        (uint8_t)transaction_messages.size(),
        connected,
        connection_state_change_time,
        last_ping_sent,
        pong_deadline);
#endif

    was_connected = connected;

    if (!connected) {
        status_notifications.clear();
        messages.clear();
        next_response = QueueItem{};
        return;
    }

    /*
    A value of 0 disables client side websocket
    Ping / Pong. In this case there is either no ping /
    pong or the server initiates the ping and client
    responds with Pong.
    */
    if ((getIntConfigUnsigned(ConfigKey::WebSocketPingInterval) != 0)
        && deadline_elapsed(next_ping_deadline)
        && platform_ws_send_ping(platform_ctx)) {
        last_ping_sent = platform_now_ms();
        next_ping_deadline = last_ping_sent + getIntConfigUnsigned(ConfigKey::WebSocketPingInterval) * 1000;
    }

    if (getIntConfigUnsigned(ConfigKey::WebSocketPingInterval) != 0 && deadline_elapsed(pong_deadline)) {
        platform_disconnect(platform_ctx);
        return;
    }

    if (next_response.is_valid()) {
        log_payload("Sending response", next_response.buf.get(), next_response.len);
        if (platform_ws_send(platform_ctx, next_response.buf.get(), next_response.len))
            next_response = QueueItem{};
        // TODO: make this robust against platform_ws_send always returning false. Use a timeout?
        return;
    }

    if (message_in_flight.is_valid()) {
        if (!deadline_elapsed(message_timeout_deadline))
            return;

        if (is_transaction_related(message_in_flight.action)) {
            log_info("%s (id %" PRIu64 ") timed out, but is transaction related. Resending.", CallActionStrings[(size_t) message_in_flight.action], message_in_flight.message_id);
            // Don't drop transaction related messages. Push to front to keep in order.
            transaction_messages.push_front(std::move(message_in_flight));
        } else {
            log_info("%s (id %" PRIu64 ") timed out. Dropping", CallActionStrings[(size_t) message_in_flight.action], message_in_flight.message_id);
            cp->onTimeout(message_in_flight.action, message_in_flight.message_id, message_in_flight.connector_id);
            message_in_flight = QueueItem{};
        }
    } else {
        if (transaction_message_retry_deadline != 0 && !deadline_elapsed(transaction_message_retry_deadline))
            return;
    }

    std::deque<QueueItem> *to_pop = nullptr;
    /*
    The Charge Point SHOULD deliver transaction-related messages to the Central System in chronological order as
    soon as possible.
    */
    // TODO: do we want to prioritize status notifications over other messages or the other way around?
    if (!transaction_messages.empty()) {
        to_pop = &transaction_messages;
    } else if (!status_notifications.empty()) {
        to_pop = &status_notifications;
    } else if (!messages.empty()) {
        to_pop = &messages;
    } else
        return;

    {
        // Limit to_send scope because the item is popped from the queue below.
        QueueItem *to_send = &to_pop->front();

        auto new_deadline = set_deadline(1000 * (is_transaction_related(to_send->action) ?
                                                            getIntConfigUnsigned(ConfigKey::TransactionMessageRetryInterval) :
                                                            getIntConfigUnsigned(ConfigKey::MessageTimeout)));

        log_payload("Sending request", to_send->buf.get(), to_send->len);
        if (!platform_ws_send(platform_ctx, to_send->buf.get(), to_send->len))
            return;

        log_info("Sent %s (id %" PRIu64 ")", CallActionStrings[(size_t) to_send->action], to_send->message_id);
        this->message_timeout_deadline = new_deadline;
    }

    message_in_flight = std::move(to_pop->front());
    to_pop->pop_front();
}


QueueItem::QueueItem(const ICall &call, int32_t connector_id) :
        action(call.action),
        buf(nullptr),
        message_id(call.ocppJmessageId),
        connector_id(connector_id),
        len(0) {
    auto length = call.measureJson();
    // TFJson will write a null terminator if the buffer is big enough.
    this->buf = heap_alloc_array<char>(length + 1);
    call.serializeJson(this->buf.get(), length + 1);
    this->len = length;
}

bool QueueItem::is_valid()
{
    return buf != nullptr;
}

static const char *lookup = "0123456789ABCDEFabcdef";

static uint8_t hex_digit_to_byte(char digit)
{
    for (size_t i = 0; i < strlen(lookup); ++i) {
        if (lookup[i] == digit)
            return i > 15 ? (i - 6) : i;
    }
    return 0xFF;
}

void* OcppConnection::start(const char *websocket_endpoint_url, const char *charge_point_name_percent_encoded, const char *basic_auth_user, const uint8_t *basic_auth_pass, size_t basic_auth_pass_length, BasicAuthPassType basic_auth_pass_type, OcppChargePoint *ocpp_handle) {
    this->cp = ocpp_handle;
    std::string ws_url;
    ws_url.reserve(strlen(websocket_endpoint_url) + 1 + strlen(charge_point_name_percent_encoded));
    ws_url += websocket_endpoint_url;
    ws_url += '/';
    ws_url += charge_point_name_percent_encoded;

    size_t cred_alloc_count = 0;
    switch (basic_auth_pass_type) {
        case BasicAuthPassType::HEX_CHARS:
        case BasicAuthPassType::TEXT:
            cred_alloc_count = 1;
            break;
        case BasicAuthPassType::TRY_BOTH:
            cred_alloc_count = 2;
            break;
        case BasicAuthPassType::NONE:
            cred_alloc_count = 0;
            break;
    }

    size_t cred_used_count = 0;
    if (cred_alloc_count > 0) {
        this->basic_auth_credentials = heap_alloc_array<BasicAuthCredentials>(cred_alloc_count);

        if (basic_auth_pass_type == BasicAuthPassType::HEX_CHARS || basic_auth_pass_type == BasicAuthPassType::TRY_BOTH) {
            bool pass_is_hex = basic_auth_pass_length == 40;
            if (pass_is_hex) {
                for (size_t i = 0; i < 40; ++i) {
                    if (!isxdigit(basic_auth_pass[i])) {
                        pass_is_hex = false;
                        break;
                    }
                }
            }

            if (!pass_is_hex && basic_auth_pass_type == BasicAuthPassType::HEX_CHARS)
                return nullptr;

            if (pass_is_hex) {
                // FIXME: Currently there is no way to stop a connection. If we add this, this memory has to be freed!
                this->basic_auth_credentials[cred_used_count].user = strdup(basic_auth_user);
                this->basic_auth_credentials[cred_used_count].pass = new uint8_t[20];
                for (size_t i = 0; i < 20; ++i) {
                    this->basic_auth_credentials[cred_used_count].pass[i] = hex_digit_to_byte(basic_auth_pass[2 * i]) << 4 | hex_digit_to_byte(basic_auth_pass[2 * i + 1]);
                }
                this->basic_auth_credentials[cred_used_count].pass_length = 20;
                ++cred_used_count;
            }
        }

        if (basic_auth_pass_type == BasicAuthPassType::TEXT || basic_auth_pass_type == BasicAuthPassType::TRY_BOTH) {
            // FIXME: Currently there is no way to stop a connection. If we add this, this memory has to be freed!
            this->basic_auth_credentials[cred_used_count].user = strdup(basic_auth_user);
            this->basic_auth_credentials[cred_used_count].pass = new uint8_t[basic_auth_pass_length];
            memcpy(this->basic_auth_credentials[cred_used_count].pass, basic_auth_pass, basic_auth_pass_length);
            this->basic_auth_credentials[cred_used_count].pass_length = basic_auth_pass_length;
            ++cred_used_count;
        }
    }

    platform_ctx = platform_init(ws_url.c_str(), this->basic_auth_credentials.get(), cred_used_count);
    if (platform_ctx == nullptr)
        return nullptr;

    platform_ws_register_receive_callback(platform_ctx, [](char *c, size_t s, void *user_data){((OcppConnection*)user_data)->handleMessage(c, s);}, this);
    platform_ws_register_pong_callback(platform_ctx, [](void *user_data){((OcppConnection*)user_data)->pong_deadline = set_deadline(OCPP_WEBSOCKET_PING_PONG_TIMEOUT * 1000);}, this);

    return platform_ctx;
}
