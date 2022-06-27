#include "OcppChargePoint.h"

#include "OcppDefines.h"
#include "OcppTypes.h"
#include "OcppConfiguration.h"
#include "OcppConnector.h"

extern "C" {
    #include "libiso8601/iso8601.h"
}

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
Connector connectors[NUM_CONNECTORS];
StatusNotificationStatus last_connector_status[NUM_CONNECTORS];

void OcppChargePoint::tick_power_on() {
    if (last_bn_send_ms == 0 && !platform_ws_connected(platform_ctx))
        return;
    if (last_bn_send_ms != 0 && !deadline_elapsed(last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval)))
        return;

    platform_printfln("Sending boot notification. %u %u %u %u", platform_now_ms(), last_bn_send_ms, last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval), 1000 * getIntConfig(ConfigKey::HeartbeatInterval));

    last_bn_send_ms = platform_now_ms();

    this->sendCallAction(CallAction::BOOT_NOTIFICATION, BootNotification("Warp 2 Charger Pro", "Tinkerforge GmbH", "warp2-X8A"));
}

void OcppChargePoint::tick_idle() {
    if (!deadline_elapsed(last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval)))
        return;

    last_bn_send_ms = platform_now_ms();

    platform_printfln("Sending heartbeat. %u %u %u %u", platform_now_ms(), last_bn_send_ms, last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval), 1000 * getIntConfig(ConfigKey::HeartbeatInterval));

    this->sendCallAction(CallAction::HEARTBEAT, Heartbeat());
}

void OcppChargePoint::tick() {
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

    for(size_t i = 0; i < ARRAY_SIZE(connectors); ++i) {
        connectors[i].tick();
    }
    connection.tick();
}

void OcppChargePoint::onConnect()
{
    if (state != OcppState::Idle)
        return;

    for(size_t i = 0; i < ARRAY_SIZE(connectors); ++i)
        connectors[i].sendStatus(connectors[i].getStatus());
}

void OcppChargePoint::onDisconnect()
{

}

bool OcppChargePoint::sendCallAction(CallAction action, const DynamicJsonDocument &doc)
{
    // Filter messages here
    connection.sendCallAction(action, doc);
    return true;
}

CallResponse OcppChargePoint::handleAuthorizeResponse(uint32_t messageId, AuthorizeResponseView conf)
{
    IdTagInfo info;
    info.updateFromIdTagInfo(conf.idTagInfo());

    for(size_t i = 0; i < NUM_CONNECTORS; ++i) {
        if (connectors[i].waiting_for_message_id != messageId)
            continue;
        connectors[i].waiting_for_message_id = 0;
        connectors[i].onAuthorizeConf(info);
        break;
    }

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleBootNotificationResponse(uint32_t messageId, BootNotificationResponseView conf) {
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

            this->sendCallAction(CallAction::STATUS_NOTIFICATION, StatusNotification(0, StatusNotificationErrorCode::NO_ERROR, StatusNotificationStatus::AVAILABLE, nullptr, platform_get_system_time(platform_ctx)));

            for(size_t i = 0; i < ARRAY_SIZE(connectors); ++i)
                connectors[i].sendStatus(connectors[i].getStatus());
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

CallResponse OcppChargePoint::handleChangeAvailability(const char *uid, ChangeAvailabilityView req)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse OcppChargePoint::handleChangeConfiguration(const char *uid, ChangeConfigurationView req)
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

    connection.sendCallResponse(ChangeConfigurationResponse(uid, status));

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleClearCache(const char *uid, ClearCacheView req)
{
    /* Errata 4.0 3.23:
    In OCPP 1.6, the Cache is not required, but the message: ClearCache.req is required to be implemented. OCPP
    does not define what the expected behaviour is.
    Additional text
    When the Authorization Cache is not implemented and the Charge Point receives a ClearCache.req message. The Charge Point
    SHALL response with ClearCache.conf with the status: Rejected.
    */
    connection.sendCallResponse(ClearCacheResponse(uid, ResponseStatus::REJECTED));

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleDataTransfer(const char *uid, DataTransferView req)
{
    /*
    If the recipient of the request has no implementation for the specific vendorId it SHALL return a status
    ‘UnknownVendor’ and the data element SHALL not be present.
    */
    connection.sendCallResponse(DataTransferResponse(uid, DataTransferResponseStatus::UNKNOWN_VENDOR_ID));

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleDataTransferResponse(uint32_t messageId, DataTransferResponseView conf)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse OcppChargePoint::handleGetConfiguration(const char *uid, GetConfigurationView req)
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

    //auto *known = (GetConfigurationResponseConfigurationKey *)malloc(sizeof(GetConfigurationResponseConfigurationKey) * known_keys);
    auto *known = new GetConfigurationResponseConfigurationKey[known_keys]();
    size_t known_idx = 0;

    auto *unknown = (const char **)malloc(sizeof(const char *) * unknown_keys);
    size_t unknown_idx = 0;

    // Scratch buffer used for strings created from int config values.
    auto *scratch_buf = (char *)malloc(sizeof(char) * scratch_buf_size);
    size_t scratch_buf_idx = 0;

    if (dump_all) {
        for(size_t i = 0; i < known_keys; ++i) {
            const char *config_value;
            OcppConfiguration &config = getConfig(i);
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
            known[known_idx].key = config_keys[i];
            known[known_idx].readonly = config.readonly;
            known[known_idx].value = config_value;
            ++known_idx;
        }
    } else {
        for(size_t i = 0; i < req.key_count(); ++i) {
            size_t result = i;
            if (lookup_key(&result, req.key(i).get(), config_keys, ARRAY_SIZE(config_keys))) {
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
    }

    connection.sendCallResponse(GetConfigurationResponse(uid, known, known_keys, unknown, unknown_keys));

    delete[] known;
    free(unknown);
    free(scratch_buf);

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleHeartbeatResponse(uint32_t messageId, HeartbeatResponseView conf)
{
    platform_set_system_time(platform_ctx, conf.currentTime());
    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleMeterValuesResponse(uint32_t messageId, MeterValuesResponseView conf)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse OcppChargePoint::handleRemoteStartTransaction(const char *uid, RemoteStartTransactionView req)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse OcppChargePoint::handleRemoteStopTransaction(const char *uid, RemoteStopTransactionView req)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse OcppChargePoint::handleReset(const char *uid, ResetView req)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse OcppChargePoint::handleStartTransactionResponse(uint32_t messageId, StartTransactionResponseView conf)
{
    IdTagInfo info;
    info.updateFromIdTagInfo(conf.idTagInfo());

    for(size_t i = 0; i < NUM_CONNECTORS; ++i) {
        if (connectors[i].waiting_for_message_id != messageId)
            continue;
        connectors[i].waiting_for_message_id = 0;
        connectors[i].onStartTransactionConf(info, conf.transactionId());
        break;
    }

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleStatusNotificationResponse(uint32_t messageId, StatusNotificationResponseView conf)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse OcppChargePoint::handleStopTransactionResponse(uint32_t messageId, StopTransactionResponseView conf)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

CallResponse OcppChargePoint::handleUnlockConnector(const char *uid, UnlockConnectorView req)
{
    return CallResponse{CallErrorCode::InternalError, "not implemented yet!"};
}

void OcppChargePoint::handleTagSeen(int32_t connectorId, const char *tagId)
{
    platform_printfln("Seen tag %s at connector %d. State is %d", tagId, connectorId, (int)this->state);

    if (connectorId > ARRAY_SIZE(connectors))
        return;

    // Don't allow tags at connector 0 (i.e. the charge point itself)
    if (connectorId <= 0)
        return;

    auto &conn = connectors[connectorId - 1];
    conn.onTagSeen(tagId);
}

void OcppChargePoint::start(const char *websocket_endpoint_url, const char *charge_point_name_percent_encoded) {
    platform_ctx = connection.start(websocket_endpoint_url, charge_point_name_percent_encoded, this);
    for(size_t i = 0; i < ARRAY_SIZE(connectors); ++i) {
        connectors[i].cp = this;
        connectors[i].connectorId = i + 1;
    }

    platform_register_tag_seen_callback(platform_ctx, [](int32_t connectorId, const char *tagId, void *user_data){((OcppChargePoint*)user_data)->handleTagSeen(connectorId, tagId);}, this);
}