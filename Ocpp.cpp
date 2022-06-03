#include "Ocpp.h"

#include "OcppDefines.h"
#include "OcppTypes.h"

extern "C" {
    #include "libiso8601/iso8601.h"
}

char send_buf[4096];

#define VALID_STATUS_STRIDE 9
bool valid_status_transitions[VALID_STATUS_STRIDE * VALID_STATUS_STRIDE] = {
/*From           To Avail  Prep   Charge SuspEV SuEVSE Finish Reserv Unavai Fault */
/*Available    */   false, true , true , true , true , false, true , true , true ,
/*Preparing    */   true , false, true , true , true , true , false, false, false,
/*Charging     */   true , false, false, true , true , true , false, true , true ,
/*SuspendedEV  */   true , false, true , false, true , true , false, true , true ,
/*SuspendedEVSE*/   true , false, true , true , false, true , false, true , true ,
/*Finishing    */   true , true , false, false, false, false, false, true , true ,
/*Reserved     */   true , true , false, false, false, false, false, true , true ,
/*Unavailable  */   true , true , true , true , true , false, false, false, true ,
/*Faulted      */   true , true , true , true , true , true , true , true , false,
};

struct Connector {
    /*For ConnectorId 0, only a limited set is applicable, namely: Available, Unavailable and Faulted.
    The status of ConnectorId 0 has no direct connection to the status of the individual Connectors (>0).*/
    int32_t connectorId;
    StatusNotificationStatus status = StatusNotificationStatus::AVAILABLE;
    IdTagInfo authorized_for;

    bool setStatus(StatusNotificationStatus newStatus) {
        if (!valid_status_transitions[(size_t)status * VALID_STATUS_STRIDE + (size_t)newStatus]) {
            platform_printfln("Invalid status transition from %s to %s (%c%c)!", StatusNotificationStatusStrings[(size_t)status], StatusNotificationStatusStrings[(size_t)newStatus], 'A'+(char)status, '1'+(char)newStatus);
            return false;
        }

        // TODO: send status notification here?
        status = newStatus;
        return true;
    }

    bool isAuthorized() {
        return authorized_for.status == ResponseIdTagInfoEntriesStatus::ACCEPTED;
    }

    void tick() {
        StatusNotificationStatus newStatus = status;

        switch(platform_get_connector_state(connectorId)) {
            case ConnectorState::NotConnected:
                switch(status) {
                    case StatusNotificationStatus::RESERVED:
                        // Stay in reserved until the central says otherwise or we see the expected idTag
                        break;
                    case StatusNotificationStatus::UNAVAILABLE:
                        // Stay unavailable until the central says otherwise
                        break;
                    default:
                        // B1 - I1
                        newStatus = StatusNotificationStatus::AVAILABLE;
                        break;
                }
                break;
            case ConnectorState::Connected:
                switch(status) {
                    case StatusNotificationStatus::AVAILABLE:
                        // A2
                        newStatus = StatusNotificationStatus::PREPARING;
                        break;
                    case StatusNotificationStatus::PREPARING:
                        // We are not ready to charge, stay in preparing
                        break;
                    case StatusNotificationStatus::CHARGING:
                        // C5
                        newStatus = StatusNotificationStatus::SUSPENDED_EVSE;
                        break;
                    case StatusNotificationStatus::SUSPENDED_EV:
                        // D5
                        newStatus = StatusNotificationStatus::SUSPENDED_EVSE;
                        break;
                    case StatusNotificationStatus::SUSPENDED_EVSE:
                        // We were suspended already, stay here.
                        break;
                    case StatusNotificationStatus::FINISHING:
                        // User still has to pull cable, stay here
                        break;
                    case StatusNotificationStatus::RESERVED:
                        // Stay in reserved, we don't know if the user will present the correct idTag
                        break;
                    case StatusNotificationStatus::UNAVAILABLE:
                        // Stay unavailable until the central says otherwise
                        break;
                    case StatusNotificationStatus::FAULTED:
                        // I2
                        newStatus = StatusNotificationStatus::PREPARING;
                        break;
                }
                break;
            case ConnectorState::ReadyToCharge:
                switch(status) {
                    case StatusNotificationStatus::SUSPENDED_EV:
                        // Still waiting for the EV...
                        break;
                    case StatusNotificationStatus::FINISHING:
                        // This should not be possible: The connector allows charging but we should have blocked it to get into the finishing state.
                        platform_printfln("Unexpected connector state \"Ready to charge\": We are finishing a transaction?!?");
                        break;
                    case StatusNotificationStatus::RESERVED:
                        // This should not be possible: The connector is reserved (and thus blocked!), we are still waiting for the idTag.
                        platform_printfln("Unexpected connector state \"Ready to charge\": Connector is reserved but idTag was not seen yet?!?");
                        break;
                    case StatusNotificationStatus::UNAVAILABLE:
                        // Stay unavailable until the central says otherwise
                        platform_printfln("Unexpected connector state \"Ready to charge\": Connector is unavailable?!?");
                        break;
                    default:
                        // A4-I4
                        newStatus = StatusNotificationStatus::SUSPENDED_EV;
                        break;
                }
                break;
            case ConnectorState::Charging:
                switch(status) {
                    case StatusNotificationStatus::FINISHING:
                        // This should not be possible: The connector allows charging but we should have blocked it to get into the finishing state.
                        platform_printfln("Unexpected connector state \"Charging\": We are finishing a transaction?!?");
                        break;
                    case StatusNotificationStatus::RESERVED:
                        // This should not be possible: The connector is reserved (and thus blocked!), we are still waiting for the idTag.
                        platform_printfln("Unexpected connector state \"Charging\": Connector is reserved but idTag was not seen yet?!?");
                        break;
                    case StatusNotificationStatus::UNAVAILABLE:
                        platform_printfln("Unexpected connector state \"Charging\": Connector is unavailable?!?");
                        break;
                    default:
                        // A3-I3
                        newStatus = StatusNotificationStatus::CHARGING;
                        break;
                }
                break;
            case ConnectorState::Faulted:
                newStatus = StatusNotificationStatus::FAULTED;
                break;
        }

        if (newStatus != status)
            setStatus(newStatus);
    }
};

/*
Connectors numbering (ConnectorIds) MUST be as follows:
• ID of the first connector MUST be 1
• Additional connectors MUST be sequentially numbered (no numbers may be skipped)
• ConnectorIds MUST never be higher than the total number of connectors of a Charge Point
• For operations intiated by the Central System, ConnectorId 0 is reserved for addressing the entire Charge
Point.
• For operations initiated by the Charge Point (when reporting), ConnectorId 0 is reserved for the Charge
Point main controller.
*/
Connector connectors[NUM_CONNECTORS + 1];


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

    for(size_t i = 1; i < ARRAY_SIZE(connectors); ++i)
        connectors[i].tick();
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
    idle_info.lastSeenTag.updateFromIdTagInfo(conf.idTagInfo());

    switch (idle_info.lastSeenTag.status) {
        case ResponseIdTagInfoEntriesStatus::ACCEPTED:
            state = OcppState::WaitingForConnectorSelection;
            platform_select_connector();
        break;

        case ResponseIdTagInfoEntriesStatus::BLOCKED:
            platform_tag_rejected(idle_info.lastSeenTag.tagId, TagRejectionType::Blocked);
            break;
        case ResponseIdTagInfoEntriesStatus::CONCURRENT_TX:
            platform_tag_rejected(idle_info.lastSeenTag.tagId, TagRejectionType::ConcurrentTx);
            break;
        case ResponseIdTagInfoEntriesStatus::EXPIRED:
            platform_tag_rejected(idle_info.lastSeenTag.tagId, TagRejectionType::Expired);
            break;
        case ResponseIdTagInfoEntriesStatus::INVALID:
            platform_tag_rejected(idle_info.lastSeenTag.tagId, TagRejectionType::Invalid);
            break;
    }

    return CallResponse{CallErrorCode::OK, ""};
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
    ClearCacheResponse(&doc, uid, ResponseStatus::REJECTED);

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
    if (conf.idTagInfo().status() != ResponseIdTagInfoEntriesStatus::ACCEPTED) {
        // TODO: Do we have to handle this case if we don't support offline authorization?
        return CallResponse{CallErrorCode::OK, ""};
    }

    connectors[wstc_info.connectorId].authorized_for.updateTagId(wstc_info.lastSeenTag.tagId);
    connectors[wstc_info.connectorId].authorized_for.updateFromIdTagInfo(conf.idTagInfo());



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
    platform_printfln("Received call error %s (%d): %s", CallErrorCodeStrings[(size_t)code], desc, details_string.c_str());
}

void Ocpp::handleTagSeen(const char *tagId)
{
    /*
    To handle here:
    - Second time a tag is seen while not charging for the tag (i.e. user does not want to start charging anymore)
    - Tag used to stop charging
    */

    if (this->state != OcppState::Idle)
        return;

    idle_info.lastSeenTag.updateTagId(tagId);

    DynamicJsonDocument doc{0};
    Authorize(&doc, tagId);
    last_call_action = CallAction::AUTHORIZE;
    size_t written = serializeJson(doc, send_buf);

    platform_ws_send(platform_ctx, send_buf, written);
}

void Ocpp::handleConnectorSelection(int32_t connectorId) {
    if (this->state != OcppState::WaitingForConnectorSelection)
        return;

    if (connectorId > NUM_CONNECTORS || connectorId < 1) {
        platform_printfln("Selected invalid connector id %u", connectorId);
        return;
    }

    wstc_info.lastSeenTag = idle_info.lastSeenTag;
    wstc_info.connectorId = connectorId;
    this->state = OcppState::WaitingForStartTransactionConf;

    DynamicJsonDocument doc{0};
    StartTransaction(&doc, connectorId, this->idle_info.lastSeenTag.tagId, platform_get_energy(connectorId), platform_get_system_time(platform_ctx));
    last_call_action = CallAction::START_TRANSACTION;
    size_t written = serializeJson(doc, send_buf);

    platform_ws_send(platform_ctx, send_buf, written);
}
