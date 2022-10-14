#include "OcppConnection.h"

#include "OcppChargePoint.h"
#include "OcppPersistency.h"
#include "OcppTypes.h"
#include <TFJson.h>

static bool is_transaction_related(CallAction action) {
     // TODO: only "periodic or clock-aligned MeterValues.req messages" are transaction related. Are those all MeterValues messages?
    return action == CallAction::START_TRANSACTION
        || action == CallAction::STOP_TRANSACTION
        || action == CallAction::METER_VALUES;
}

void OcppConnection::handleMessage(char *message, size_t message_len)
{
    platform_printfln("Received message %.*s", (int)std::min(message_len, (size_t)40), message);
    DynamicJsonDocument doc{4096};
    // TODO: we should use
    // https://arduinojson.org/v6/how-to/deserialize-a-very-large-document/#deserialization-in-chunks
    // to parse each member in the top level array by its own.
    // This would allow us to send CallErrors back to the central if
    // we receive a message that for example can not be completely
    // parsed as JSON.
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        platform_printfln("deserializeJson() failed: %s", error.c_str());
        return;
    }
    doc.shrinkToFit();

    if (!doc.is<JsonArray>()) {
        platform_printfln("deserialized JSON is not an array at top level");
        return;
    }

    if (!doc[0].is<int32_t>()) {
        platform_printfln("deserialized JSON array does not start with message type ID");
        return;
    }

    if (!doc[1].is<const char *>()) {
        platform_printfln("deserialized JSON array does not contain unique ID as second member ");
        return;
    }

    int32_t messageType = doc[0];
    const char *uniqueID = doc[1];

    if (messageType == (int32_t)OcppRpcMessageType::CALL) {
        if (doc.size() != 4) {
            platform_printfln("received call with %d members, but expected 4.", (int)doc.size());
            return;
        }

        if (!doc[2].is<const char *>()) {
            platform_printfln("received call with action not being a string.");
            return;
        }

        if (doc[3].isNull() || !doc[3].is<JsonObject>()) {
            platform_printfln("received call with payload being neither an object nor null.");
            return;
        }

        if (cp->state == OcppState::Rejected) {
            // "While Rejected, the Charge Point SHALL NOT respond to any Central System initiated message. the Central System SHOULD NOT initiate any."
            platform_printfln("received call while being rejected. Ignoring call.");
            return;
        }

        CallResponse res = callHandler(uniqueID, doc[2].as<const char *>(), doc[3].as<JsonObject>(), cp);
        if (res.result != CallErrorCode::OK)
            sendCallError(uniqueID, res.result, res.error_description);
        // TODO handle responses here?
        return;
    }

    if (messageType != (int32_t)OcppRpcMessageType::CALLRESULT && messageType != (int32_t)OcppRpcMessageType::CALLERROR) {
        platform_printfln("received unknown message type %d", messageType);
        return;
    }

    long uid_long = atol(uniqueID);
    if (uid_long < 0) {
        platform_printfln("received %s with invalid message ID %ld. ", messageType == 3 ? "call result" : "call error", uid_long);
        return;
    }
    uint32_t uid = (uint32_t) uid_long;

    if (!message_in_flight.is_valid()) {
        platform_printfln("received %s with message ID %u, but no call is in flight", messageType == 3 ? "call result" : "call error", uid);
    }

    uint32_t last_call_message_id = message_in_flight.message_id;

    if (uid != last_call_message_id) {
        platform_printfln("received %s with message ID %u. expected was %u ", messageType == 3 ? "call result" : "call error", uid, last_call_message_id);
        return;
    }

    if (messageType == (int32_t)OcppRpcMessageType::CALLRESULT) {
        if (doc.size() != 3) {
            platform_printfln("received call result with %d members, but expected 3.", (int)doc.size());
            return;
        }

        if (doc[2].isNull() || !doc[2].is<JsonObject>()) {
            platform_printfln("received call result with payload being neither an object nor null.");
            return;
        }

        if (is_transaction_related(message_in_flight.action))
            onTxnMsgResponseReceived(message_in_flight.timestamp);

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
            platform_printfln("received call error with %d members, but expected 5.", (int)doc.size());
            return;
        }

        if (!doc[2].is<const char *>()) {
            platform_printfln("received call error with error code not being a string!.");
            return;
        }

        if (!doc[3].is<const char *>()) {
            platform_printfln("received call error with error description not being a string!.");
            return;
        }

        if (!doc[4].is<JsonObject>()) {
            platform_printfln("received call error with error details not being an object!.");
            return;
        }

        size_t cec = (size_t) CallErrorCode::GenericError;
        if (!lookup_key(&cec, doc[2], CallErrorCodeStrings, (size_t)CallErrorCode::OK, CallErrorCodeStringAliases, CallErrorCodeStringAliasIndices, CallErrorCodeStringAliasLength)) {
            platform_printfln("received call error with unknown error code '%s'! Replacing with GenericError.", doc[2].as<const char *>());
        }

        handleCallError((CallErrorCode)cec, doc[3], doc[4]);
        return;
    }
}

void OcppConnection::handleCallError(CallErrorCode code, const char *desc, JsonObject details)
{
    std::string details_string;
    serializeJsonPretty(details, details_string);
    platform_printfln("Received call error %s %s: %s", CallErrorCodeStrings[(size_t)code], desc, details_string.c_str());

    if (!is_transaction_related(message_in_flight.action)) {
        cp->onTimeout(message_in_flight.action, message_in_flight.message_id, message_in_flight.connector_id);
        message_in_flight = QueueItem{};
        return;
    }

    transaction_messages.push_front(std::move(message_in_flight));
    ++transaction_message_attempts;
    if (transaction_message_attempts >= getIntConfig(ConfigKey::TransactionMessageAttempts)) {
        cp->onTimeout(message_in_flight.action, message_in_flight.message_id, message_in_flight.connector_id);
        message_in_flight = QueueItem{};
        return;
    }

    transaction_message_retry_deadline = platform_now_ms() + transaction_message_attempts * getIntConfig(ConfigKey::TransactionMessageRetryInterval) * 1000;
}

static size_t buildCallError(TFJsonSerializer &json, const char *uid, CallErrorCode code, const char *desc) {
    json.addArray();
    json.add((int32_t)OcppRpcMessageType::CALLERROR);
    json.add(uid);
    json.add(CallErrorCodeStrings[(size_t)code]);
    json.add(desc);
    json.addObject();
    json.endObject();
    json.endArray();
    return json.end();
}

void OcppConnection::sendCallError(const char *uid, CallErrorCode code, const char *desc)
{
    size_t len = 0;
    {
        TFJsonSerializer json{nullptr, 0};
        len = buildCallError(json, uid, code, desc);
    }
    auto buf = heap_alloc_array<char>(len);
    TFJsonSerializer json{buf.get(), len};
    len = buildCallError(json, uid, code, desc);

    platform_ws_send(platform_ctx, buf.get(), len);
}

bool OcppConnection::sendCallResponse(const ICall &call)
{
    auto len = call.measureJson();
    auto buf = heap_alloc_array<char>(len);
    call.serializeJson(buf.get(), len);

    platform_ws_send(platform_ctx, buf.get(), len);
    return true;
}

bool OcppConnection::sendCallAction(const ICall &call, time_t timestamp, int32_t connectorId)
{
    // Not transaction-related messages are allowed to be dropped.
    // This means that we can just enforce a queue depth of 5.
    // Transaction-related messages may not be dropped, however we (currently) allow dropping meter values messages.
    // This means that only Start and StopTxn messages will be emplaced in the txn_msg queue.
    // To create a StartTxn message the connection must work, because we don't implement any way of authenticating offline.
    // So only if the Authorize message gets through, we can create a StartTxn message.
    // The worst case now is that we lose the connection directly after the Authorize message, and the transaction continues.
    // We then have enqueued at most two messages, one StartTxn and one StopTxn.
    if (is_transaction_related(call.action)) {
        if (timestamp == 0) {
            platform_printfln("Attempted to send transaction related call action without valid timestamp!");
            return false;
        }
        if (transaction_messages.size() >= 5) {
            size_t i;
            for(i = 0; i < transaction_messages.size(); ++i)
                if (transaction_messages[i].action == CallAction::METER_VALUES)
                    break;
            transaction_messages.erase(transaction_messages.begin() + i);
        }
        transaction_messages.emplace_back(call, timestamp, connectorId);
        return true;
    }

    // drop not transaction related messages and status notifications:
    // "The Charge Point MAY send a StatusNotification.req PDU to report an error that occurred while the Charge Point was offline."
    if (!platform_ws_connected(platform_ctx))
        return false;

    if (call.action == CallAction::STATUS_NOTIFICATION) {
        if (status_notifications.size() > 5)
            status_notifications.pop_front();
        status_notifications.emplace_back(call, timestamp, connectorId);
    }
    else {
        if (messages.size() > 5)
            messages.pop_front();
        messages.emplace_back(call, timestamp, connectorId);
    }

    return true;
}

void OcppConnection::tick() {
    static bool was_connected = false;
    bool connected = platform_ws_connected(platform_ctx);
    if (!connected && was_connected)
        cp->onDisconnect();
    else if (connected && !was_connected)
        cp->onConnect();

    was_connected = connected;

    if (!connected) {
        status_notifications.clear();
        messages.clear();
        return;
    }


    if (message_in_flight.is_valid()) {
        if (!deadline_elapsed(message_timeout_deadline))
            return;

        if (is_transaction_related(message_in_flight.action)) {
            // Don't drop transaction related messages. Push to front to keep in order.
            transaction_messages.push_front(std::move(message_in_flight));
        } else {
            cp->onTimeout(message_in_flight.action, message_in_flight.message_id, message_in_flight.connector_id);
            message_in_flight = QueueItem{};
        }
    }

    /*
    The Charge Point SHOULD deliver transaction-related messages to the Central System in chronological order as
    soon as possible.
    */
    // TODO: do we want to prioritize status notifications over other messages or the other way around?
    if (!transaction_messages.empty()) {
        message_in_flight = std::move(transaction_messages.front());
        transaction_messages.pop_front();
    } else if (!status_notifications.empty()) {
        message_in_flight = std::move(status_notifications.front());
        status_notifications.pop_front();
    } else if (!messages.empty()) {
        message_in_flight = std::move(messages.front());
        messages.pop_front();
    } else
        return;

    message_timeout_deadline = platform_now_ms() + getIntConfig(ConfigKey::MessageTimeout) * 1000;

    platform_ws_send(platform_ctx, message_in_flight.buf.get(), message_in_flight.len);
}
