#include "Ocpp.h"

#include "OcppDefines.h"
#include "OcppTypes.h"

extern "C" {
    #include "libiso8601/iso8601.h"
}

char send_buf[4096];

void Ocpp::tick_power_on() {
    if ((last_bn_send_ms == 0 && !platform_ws_connected(platform_ctx)) || !deadline_elapsed(last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval)))
        return;

    platform_printfln("Sending boot notification. %u %u %u", last_bn_send_ms, last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval), 1000 * getIntConfig(ConfigKey::HeartbeatInterval));

    last_bn_send_ms = platform_now_ms();

    DynamicJsonDocument doc{0};
    BootNotification(&doc, "Warp 2 Charger Pro", "Tinkerforge GmbH", "warp2-X8A");
    last_call_action = CallAction::BOOT_NOTIFICATION;
    size_t written = serializeJson(doc, send_buf);

    platform_ws_send(platform_ctx, send_buf, written);
}

void Ocpp::tick_idle() {
    if (!deadline_elapsed(last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval)))
        return;

    last_bn_send_ms = platform_now_ms();

    DynamicJsonDocument doc{0};
    Heartbeat(&doc);
    last_call_action = CallAction::HEARTBEAT;
    size_t written = serializeJson(doc, send_buf);

    platform_ws_send(platform_ctx, send_buf, written);
}

void Ocpp::tick() {
    switch (state) {
        case OcppState::PowerOn:
        case OcppState::Pending:
        case OcppState::Rejected:
            tick_power_on();
            break;
        case OcppState::Idle:
            tick_idle();
            break;
    }
}

void Ocpp::handleMessage(char *message, size_t message_len)
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

        if (this->state == OcppState::Rejected) {
            // "While Rejected, the Charge Point SHALL NOT respond to any Central System initiated message. the Central System SHOULD NOT initiate any."
            platform_printfln("received call while being rejected. Ignoring call.");
            return;
        }

        CallResponse res = callHandler(uniqueID, doc[2].as<const char *>(), doc[3].as<JsonObject>(), this);
        if (res.result != CallErrorCode::OK)
            sendCallError(uniqueID, res.result, res.error_description, JsonObject());
        // TODO handle responses here?
        return;
    }

    if (messageType != 3 && messageType != 4) {
        platform_printfln("received unknown message type %d", messageType);
        return;
    }

    long uid = atol(uniqueID);
    if (uid != last_call_message_id) {
        platform_printfln("received %s with message ID %d. expected was %u ", messageType == 3 ? "call result" : "call error", uid, last_call_message_id);
        return;
    }

    if (messageType == (int32_t)OcppRpcMessageType::CALLRESULT) {
        if (doc.size() != 3) {
            platform_printfln("received call result with %d members, but expected 3.", doc.size());
            return;
        }
        // TODO: check call_id!

        if (doc[2].isNull() || !doc[2].is<JsonObject>()) {
            platform_printfln("received call result with payload being neither an object nor null.", doc.size());
            return;
        }

        CallResponse res = callResultHandler(last_call_action, doc[2].as<JsonObject>(), this);
        /*if (res.result != CallErrorCode::OK)
            sendCallError(uniqueID, res.result, res.error_description, JsonObject());*/
        return;
    }

    if (messageType == (int32_t)OcppRpcMessageType::CALLERROR) {
        if (doc.size() != 5) {
            platform_printfln("received call error with %d members, but expected 5.", doc.size());
            return;
        }

        // TODO: check call_id!

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

void Ocpp::sendCallError(const char *uid, CallErrorCode code, const char *desc, JsonObject details)
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

CallResponse Ocpp::handleAuthorizeResponse(AuthorizeResponseView conf)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse Ocpp::handleBootNotificationResponse(BootNotificationResponseView conf) {
    if ((state != OcppState::PowerOn) &&
        (state != OcppState::Pending) &&
        (state != OcppState::Rejected))
        return CallResponse{CallErrorCode::InternalError, "unexpected state"};

    // We can set the heartbeat interval in any case, as it is only relevant (for heartbeats)
    // when "Accepted" is received.
    // In other cases this is the interval to retry sending boot notification requests with.
    if (conf.interval() > 0)
        setIntConfig(ConfigKey::HeartbeatInterval, conf.interval());
    else
        setIntConfig(ConfigKey::HeartbeatInterval, DEFAULT_BOOT_NOTIFICATION_RESEND_INTERVAL_S);

    switch (conf.status()) {
        case BootNotificationResponseStatus::ACCEPTED: {
            platform_set_system_time(platform_ctx, conf.currentTime());
            state = OcppState::Idle;
            break;
        }
        case BootNotificationResponseStatus::PENDING: {
            state = OcppState::Pending;
            break;
        }
        case BootNotificationResponseStatus::REJECTED: {
            state = OcppState::Rejected;
            break;
        }
    }

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse Ocpp::handleChangeAvailability(const char *uid, ChangeAvailabilityView req)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse Ocpp::handleChangeConfiguration(const char *uid, ChangeConfigurationView req)
{
    size_t key_idx;
    ChangeConfigurationResponseStatus status = ChangeConfigurationResponseStatus::ACCEPTED;
    if (!lookup_key(&key_idx, req.key(), config_keys, CONFIG_COUNT)) {
        status = ChangeConfigurationResponseStatus::NOT_SUPPORTED;
    } else if (getConfig(key_idx).readonly) {
        status = ChangeConfigurationResponseStatus::REJECTED;
    } else {
        status = getConfig(key_idx).setValue(req.value());
    }

    DynamicJsonDocument doc{0};
    ChangeConfigurationResponse(&doc, uid, status);

    size_t written = serializeJson(doc, send_buf);

    platform_ws_send(platform_ctx, send_buf, written);

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse Ocpp::handleClearCache(const char *uid, ClearCacheView req)
{
    /* Errata 4.0 3.23:
    In OCPP 1.6, the Cache is not required, but the message: ClearCache.req is required to be implemented. OCPP
    does not define what the expected behaviour is.
    Additional text
    When the Authorization Cache is not implemented and the Charge Point receives a ClearCache.req message. The Charge Point
    SHALL response with ClearCache.conf with the status: Rejected.
    */
    DynamicJsonDocument doc{0};
    ClearCacheResponse(&doc, uid, ClearCacheResponseStatus::REJECTED);

    size_t written = serializeJson(doc, send_buf);

    platform_ws_send(platform_ctx, send_buf, written);

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse Ocpp::handleDataTransfer(const char *uid, DataTransferView req)
{
    /*
    If the recipient of the request has no implementation for the specific vendorId it SHALL return a status
    ‘UnknownVendor’ and the data element SHALL not be present.
    */
    DynamicJsonDocument doc{0};
    DataTransferResponse(&doc, uid, DataTransferResponseStatus::UNKNOWN_VENDOR_ID);

    size_t written = serializeJson(doc, send_buf);

    platform_ws_send(platform_ctx, send_buf, written);

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse Ocpp::handleDataTransferResponse(DataTransferResponseView conf)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse Ocpp::handleGetConfiguration(const char *uid, GetConfigurationView req)
{
    size_t known_keys = 0;
    size_t scratch_buf_size = 0;
    size_t unknown_keys = 0;
    bool dump_all = false;

    if (req.key_count() == 0) {
        known_keys = CONFIG_COUNT;
        scratch_buf_size = 20 * known_keys;
        dump_all = true;
    } else {
        for(size_t i = 0; i < req.key_count(); ++i) {
            size_t result;
            if (lookup_key(&result, req.key(i).get(), config_keys, CONFIG_COUNT)) {
                ++known_keys;
                switch(getConfig(result).type) {
                    case OcppConfigurationValueType::Boolean:
                        scratch_buf_size += 0;
                        break;
                    case OcppConfigurationValueType::Integer:
                        scratch_buf_size += 20;
                        break;
                    case OcppConfigurationValueType::CSL:
                        scratch_buf_size += 0;
                        break;
                }
            }
            else
                ++unknown_keys;
        }
    }

    auto *known = (GetConfigurationResponseConfigurationKey *)malloc(sizeof(GetConfigurationResponseConfigurationKey) * known_keys);
    size_t known_idx = 0;

    auto *unknown = (const char **)malloc(sizeof(const char *) * unknown_keys);
    size_t unknown_idx = 0;

    auto *scratch_buf = (char *)malloc(sizeof(char) * scratch_buf_size);
    size_t scratch_buf_idx = 0;

    for(size_t i = 0; i < known_keys; ++i) {
        size_t result = i;
        if (dump_all || lookup_key(&result, req.key(i).get(), config_keys, ARRAY_SIZE(config_keys))) {
            const char *config_value;
            OcppConfiguration &config = getConfig(result);
            switch(config.type) {
                case OcppConfigurationValueType::Boolean:
                    config_value = config.value.boolean.b ? "true" : "false";
                    break;
                case OcppConfigurationValueType::Integer:
                    config_value = (const char *)scratch_buf + scratch_buf_idx;
                    scratch_buf_idx += snprintf(scratch_buf + scratch_buf_idx, scratch_buf_size - scratch_buf_idx, "%d", config.value.integer.i);
                    ++scratch_buf_idx; // for null terminator
                    break;
                case OcppConfigurationValueType::CSL:
                    config_value = config.value.csl.c;
                    break;
            }
            known[known_idx].key = config_keys[result];
            known[known_idx].readonly = config.readonly;
            known[known_idx].value = config_value;
            ++known_idx;
        }
        else {
            unknown[unknown_idx] = req.key(i).get();
            ++unknown_idx;
        }
    }


    DynamicJsonDocument doc{0};
    GetConfigurationResponse(&doc, uid, known, known_keys, unknown, unknown_keys);

    size_t written = serializeJson(doc, send_buf);

    platform_ws_send(platform_ctx, send_buf, written);

    free(known);
    free(unknown);
    free(scratch_buf);

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse Ocpp::handleHeartbeatResponse(HeartbeatResponseView conf)
{
    platform_set_system_time(platform_ctx, conf.currentTime());
    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse Ocpp::handleMeterValuesResponse(MeterValuesResponseView conf)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse Ocpp::handleRemoteStartTransaction(const char *uid, RemoteStartTransactionView req)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse Ocpp::handleRemoteStopTransaction(const char *uid, RemoteStopTransactionView req)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse Ocpp::handleReset(const char *uid, ResetView req)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse Ocpp::handleStartTransactionResponse(StartTransactionResponseView conf)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse Ocpp::handleStatusNotificationResponse(StatusNotificationResponseView conf)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse Ocpp::handleStopTransactionResponse(StopTransactionResponseView conf)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse Ocpp::handleUnlockConnector(const char *uid, UnlockConnectorView req)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

void Ocpp::handleCallError(CallErrorCode code, const char *desc, JsonObject details)
{
    std::string details_string;
    serializeJsonPretty(details, details_string);
    platform_printfln("Received call error %s (%d): %s %s", CallErrorCodeStrings[(size_t)code], desc, details_string.c_str());
}
