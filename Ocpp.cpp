#include "Ocpp.h"

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

void Ocpp::tick_power_on() {
    if (last_bn_send_ms == 0 && !platform_ws_connected(platform_ctx))
        return;
    if (last_bn_send_ms != 0 && !deadline_elapsed(last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval)))
        return;

    platform_printfln("Sending boot notification. %u %u %u %u", platform_now_ms(), last_bn_send_ms, last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval), 1000 * getIntConfig(ConfigKey::HeartbeatInterval));

    last_bn_send_ms = platform_now_ms();

    DynamicJsonDocument doc{0};
    BootNotification(&doc, "Warp 2 Charger Pro", "Tinkerforge GmbH", "warp2-X8A");
    connection.sendCallAction(CallAction::BOOT_NOTIFICATION, &doc);
}

void Ocpp::tick_idle() {
    if (!deadline_elapsed(last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval)))
        return;

    last_bn_send_ms = platform_now_ms();

    platform_printfln("Sending heartbeat. %u %u %u %u", platform_now_ms(), last_bn_send_ms, last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval), 1000 * getIntConfig(ConfigKey::HeartbeatInterval));

    DynamicJsonDocument doc{0};
    Heartbeat(&doc);
    connection.sendCallAction(CallAction::HEARTBEAT, &doc);
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

    for(size_t i = 0; i < ARRAY_SIZE(connectors); ++i) {
        connectors[i].tick();
/*        auto conn_status = connectors[i].getStatus();
        if (last_connector_status[i] != conn_status) {
            DynamicJsonDocument doc{0};
            StatusNotification(&doc, i + 1, StatusNotificationErrorCode::NO_ERROR, conn_status);
            connection.sendCallAction(CallAction::STATUS_NOTIFICATION, &doc);
        }*/
    }
    connection.tick();
}

void Ocpp::onConnect()
{
    if (state != OcppState::Idle)
        return;

    for(size_t i = 0; i < ARRAY_SIZE(connectors); ++i)
        connectors[i].sendStatus(connectors[i].getStatus());
}

void Ocpp::onDisconnect()
{

}

CallResponse Ocpp::handleAuthorizeResponse(AuthorizeResponseView conf)
{
    auto connectorId = idle_info.lastTagForConnector;
    if (connectorId <= 0 || connectorId > NUM_CONNECTORS)
        return CallResponse{CallErrorCode::OK, ""};

    IdTagInfo info;
    info.updateFromIdTagInfo(conf.idTagInfo());

    switch (connectors[connectorId - 1].onAuthorizeConf(info)) {
        case Connector::OTARResult::StartTransaction: {
            DynamicJsonDocument doc{0};
            StartTransaction(&doc, connectorId, info.tagId, platform_get_energy(connectorId), platform_get_system_time(platform_ctx));
            connection.sendCallAction(CallAction::START_TRANSACTION, &doc);

            break;
        }
        case Connector::OTARResult::StopTransaction: {
            DynamicJsonDocument doc{0};
            StopTransaction(&doc, platform_get_energy(connectorId), platform_get_system_time(platform_ctx), connectors[connectorId - 1].transaction_id, info.tagId, StopTransactionReason::LOCAL);
            connection.sendCallAction(CallAction::STOP_TRANSACTION, &doc);

            break;
        }
        case Connector::OTARResult::NOP:
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

            DynamicJsonDocument doc{0};
            StatusNotification(&doc, 0, StatusNotificationErrorCode::NO_ERROR, StatusNotificationStatus::AVAILABLE, nullptr, platform_get_system_time(platform_ctx));
            connection.sendCallAction(CallAction::STATUS_NOTIFICATION, &doc);

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
    connection.sendCallResponse(&doc);

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
    connection.sendCallResponse(&doc);

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
    connection.sendCallResponse(&doc);

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

    DynamicJsonDocument doc{0};
    GetConfigurationResponse(&doc, uid, known, known_keys, unknown, unknown_keys);
    connection.sendCallResponse(&doc);

    delete[] known;
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
    auto connectorId = idle_info.lastTagForConnector;
    if (connectorId <= 0 || connectorId > NUM_CONNECTORS)
        return CallResponse{CallErrorCode::OK, ""};

    IdTagInfo info;
    info.updateFromIdTagInfo(conf.idTagInfo());

    if (connectors[idle_info.lastTagForConnector - 1].onStartTransactionConf(info, conf.transactionId()) == Connector::OSTCResult::SendStopTxnDeauthed) {
        DynamicJsonDocument doc{0};
        StopTransaction(&doc, platform_get_energy(connectorId), platform_get_system_time(platform_ctx), conf.transactionId(), info.tagId, StopTransactionReason::DE_AUTHORIZED);
        connection.sendCallAction(CallAction::STOP_TRANSACTION, &doc);
    }

    return CallResponse{CallErrorCode::OK, ""};
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

void Ocpp::handleTagSeen(int32_t connectorId, const char *tagId)
{
    platform_printfln("Seen tag %s at connector %d. State is %d", tagId, connectorId, (int)this->state);

    if (connectorId > ARRAY_SIZE(connectors))
        return;

    // Don't allow tags at connector 0 (i.e. the charge point itself)
    if (connectorId <= 0)
        return;

    auto &conn = connectors[connectorId - 1];
    if (conn.onTagSeen(tagId) == Connector::OTSResult::RequestAuthentication) {
        DynamicJsonDocument doc{0};
        Authorize(&doc, tagId);
        connection.sendCallAction(CallAction::AUTHORIZE, &doc);
    }

    idle_info.lastTagForConnector = connectorId;
}

void Ocpp::start(const char *websocket_endpoint_url, const char *charge_point_name_percent_encoded) {
    platform_ctx = connection.start(websocket_endpoint_url, charge_point_name_percent_encoded, this);
    for(size_t i = 0; i < ARRAY_SIZE(connectors); ++i) {
        connectors[i].connection = &connection;
        connectors[i].connectorId = i + 1;
    }

    platform_register_tag_seen_callback(platform_ctx, [](int32_t connectorId, const char *tagId, void *user_data){((Ocpp*)user_data)->handleTagSeen(connectorId, tagId);}, this);
}
