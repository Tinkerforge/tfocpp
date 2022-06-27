#include "OcppConnection.h"

#include "OcppChargePoint.h"


char send_buf[4096];

void OcppConnection::handleMessage(char *message, size_t message_len)
{
    platform_printfln("Received message %.*s", message_len > 40 ? 40 : message_len, message);
    StaticJsonDocument<4096> doc;
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
            platform_printfln("received call with %d members, but expected 4.", doc.size());
            return;
        }

        if (!doc[2].is<const char *>()) {
            platform_printfln("received call with action not being a string.", doc.size());
            return;
        }

        if (doc[3].isNull() || !doc[3].is<JsonObject>()) {
            platform_printfln("received call with payload being neither an object nor null.", doc.size());
            return;
        }

        if (cp->state == OcppState::Rejected) {
            // "While Rejected, the Charge Point SHALL NOT respond to any Central System initiated message. the Central System SHOULD NOT initiate any."
            platform_printfln("received call while being rejected. Ignoring call.");
            return;
        }

        CallResponse res = callHandler(uniqueID, doc[2].as<const char *>(), doc[3].as<JsonObject>(), cp);
        if (res.result != CallErrorCode::OK)
            sendCallError(uniqueID, res.result, res.error_description, JsonObject());
        // TODO handle responses here?
        return;
    }

    if (messageType != (int32_t)OcppRpcMessageType::CALLRESULT && messageType != (int32_t)OcppRpcMessageType::CALLERROR) {
        platform_printfln("received unknown message type %d", messageType);
        return;
    }

    long uid = atol(uniqueID);

    if (!message_in_flight.is_valid()) {
        platform_printfln("received %s with message ID %d, but no call is in flight", messageType == 3 ? "call result" : "call error", uid);
    }

    uint32_t last_call_message_id = message_in_flight.message_id;

    if (uid != last_call_message_id) {
        platform_printfln("received %s with message ID %d. expected was %u ", messageType == 3 ? "call result" : "call error", uid, last_call_message_id);
        return;
    }

    if (messageType == (int32_t)OcppRpcMessageType::CALLRESULT) {
        if (doc.size() != 3) {
            platform_printfln("received call result with %d members, but expected 3.", doc.size());
            return;
        }

        if (doc[2].isNull() || !doc[2].is<JsonObject>()) {
            platform_printfln("received call result with payload being neither an object nor null.", doc.size());
            return;
        }

        CallResponse res = callResultHandler(uid, message_in_flight.action, doc[2].as<JsonObject>(), cp);
        message_in_flight = QueueEntry{};
        /*if (res.result != CallErrorCode::OK)
            sendCallError(uniqueID, res.result, res.error_description, JsonObject());*/
        return;
    }

    if (messageType == (int32_t)OcppRpcMessageType::CALLERROR) {
        if (doc.size() != 5) {
            platform_printfln("received call error with %d members, but expected 5.", doc.size());
            return;
        }

        if (!doc[2].is<const char *>()) {
            platform_printfln("received call error with error code not being a string!.", doc.size());
            return;
        }

        if (!doc[3].is<const char *>()) {
            platform_printfln("received call error with error description not being a string!.", doc.size());
            return;
        }

        if (!doc[4].is<JsonObject>()) {
            platform_printfln("received call error with error details not being an object!.", doc.size());
            return;
        }

        handleCallError(doc[2], doc[3], doc[4]);
        return;
    }
}

void OcppConnection::handleCallError(CallErrorCode code, const char *desc, JsonObject details)
{
    std::string details_string;
    serializeJsonPretty(details, details_string);
    platform_printfln("Received call error %s (%d): %s", CallErrorCodeStrings[(size_t)code], desc, details_string.c_str());

    //if (!is_transaction_related(message_in_flight.action))

    //TODO Don't throw away transaction related messages here. Resend them!
    message_in_flight = QueueEntry{};
}


void OcppConnection::sendCallError(const char *uid, CallErrorCode code, const char *desc, JsonObject details)
{
    DynamicJsonDocument doc{
        JSON_ARRAY_SIZE(5)
        + details.memoryUsage()
    };

    doc.add((int32_t)OcppRpcMessageType::CALLERROR);
    doc.add(uid);
    doc.add(CallErrorCodeStrings[(size_t)code]);
    doc.add(desc);
    // TODO: use JsonVariant::link when ArduinoJson 6.20 is released.
    doc.add(details);

    size_t written = serializeJson(doc, send_buf);

    platform_ws_send(platform_ctx, send_buf, written);
}

bool OcppConnection::sendCallResponse(const DynamicJsonDocument &doc)
{
    size_t written = serializeJson(doc, send_buf);

    platform_ws_send(platform_ctx, send_buf, written);
    return true;
}

bool is_transaction_related(CallAction action) {
     // TODO: only "periodic or clock-aligned MeterValues.req messages" are transaction related. Are those all MeterValues messages?
    return action == CallAction::START_TRANSACTION
        || action == CallAction::STOP_TRANSACTION
        || action == CallAction::METER_VALUES;
}
/*
bool is_transaction_related(QueueEntry entry) {
    return is_transaction_related(entry.action);
}
*/
bool OcppConnection::sendCallAction(CallAction action, const DynamicJsonDocument &doc)
{
    // TODO: handle full queues
    if (is_transaction_related(action)) {
        transaction_messages.emplace_back(action, doc);
        return true;
    }

    // drop not transaction related messages and status notifications:
    // "The Charge Point MAY send a StatusNotification.req PDU to report an error that occurred while the Charge Point was offline."
    if (!platform_ws_connected(platform_ctx))
        return false;

    if (action == CallAction::STATUS_NOTIFICATION)
        status_notifications.emplace_back(action, doc);
    else
        messages.emplace_back(action, doc);

    return true;

    /*last_call_action = action;
    size_t written = serializeJson(*doc, send_buf);

    platform_ws_send(platform_ctx, send_buf, written);
    return true;*/
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

    // TODO
    // - handle connection loss: don't send any messages, maybe filter message queue
    // - handle timeouts for message in flight

    if (message_in_flight.is_valid())
        return;

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

    platform_ws_send(platform_ctx, message_in_flight.buf.get(), message_in_flight.len);
}