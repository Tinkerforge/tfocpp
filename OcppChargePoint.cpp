#include "OcppChargePoint.h"

#include "OcppDefines.h"
#include "OcppTypes.h"
#include "OcppConfiguration.h"
#include "OcppConnector.h"

#include "OcppPersistency.h"

extern "C" {
    #include "lib/libiso8601/iso8601.h"
}


//static StatusNotificationStatus last_connector_status[NUM_CONNECTORS];

void OcppChargePoint::tick_power_on() {
    if (last_bn_send_ms == 0 && !platform_ws_connected(platform_ctx))
        return;
    if (last_bn_send_ms != 0 && !deadline_elapsed(last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval)))
        return;

    log_info("Sending BootNotification.req. %u %u %u %u", platform_now_ms(), last_bn_send_ms, last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval), 1000 * getIntConfig(ConfigKey::HeartbeatInterval));

    last_bn_send_ms = platform_now_ms();

    this->sendCallAction(BootNotification(
        platform_get_charge_point_vendor(),
        platform_get_charge_point_model(),
        platform_get_charge_point_serial_number(),
        nullptr,
        platform_get_firmware_version(),
        platform_get_iccid(),
        platform_get_imsi(),
        platform_get_meter_type(),
        platform_get_meter_serial_number()
    ));
}

void OcppChargePoint::tick_idle() {
    this->sendStatus();

    if (!deadline_elapsed(last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval)))
        return;

    last_bn_send_ms = platform_now_ms();

    log_info("Sending Heartbeat.req. %u %u %u %u", platform_now_ms(), last_bn_send_ms, last_bn_send_ms + 1000 * getIntConfig(ConfigKey::HeartbeatInterval), 1000 * getIntConfig(ConfigKey::HeartbeatInterval));

    this->sendCallAction(Heartbeat());
}

void OcppChargePoint::tick_hard_reset() {
    // TODO: if persistence is implemented: Write all txn messages to flash and reboot instantly.
    platform_reset();
}

void OcppChargePoint::tick_soft_reset() {
    // Stay in soft reset until all transaction messages are sent (to the platforms websocket impl).
    // Then stop, but allow the platform as long as it takes to send those websocket messages.
    // Then reset.

    if (connection.transaction_messages.size() != 0)
        return;

    this->stop();

    platform_reset();
}

void OcppChargePoint::tick_flush_persistent_messages() {
    if (connection.message_in_flight.is_valid())
        return;

    if (restoreNextTxnMessage(&connection))
        return;

    finishRestore();
    this->state = OcppState::Idle;
}

void OcppChargePoint::tick() {
    switch (state) {
        case OcppState::PowerOn:
        case OcppState::Pending:
        case OcppState::Rejected:
            tick_power_on();
            break;

        case OcppState::FlushPersistentMessages:
            tick_flush_persistent_messages();
            break;

        case OcppState::Idle:
        case OcppState::Unavailable:
        case OcppState::Faulted:
            tick_idle();
            break;

        case OcppState::SoftReset:
            tick_soft_reset();
            break;

        case OcppState::HardReset:
            tick_hard_reset();
            break;
    }

    if (state != OcppState::SoftReset && state != OcppState::HardReset) {
        for(int32_t i = 0; i < NUM_CONNECTORS; ++i) {
            connectors[i].tick();
        }
    }
    connection.tick();
}

void OcppChargePoint::onConnect()
{
    if (state != OcppState::Idle)
        return;

    this->forceSendStatus();
    for(int32_t i = 0; i < NUM_CONNECTORS; ++i)
        connectors[i].forceSendStatus();
}

void OcppChargePoint::onDisconnect()
{

}

ChangeAvailabilityResponseStatus OcppChargePoint::onChangeAvailability(ChangeAvailabilityType type)
{
    switch(type) {
        case ChangeAvailabilityType::OPERATIVE:
            /* In the event that Central System requests Charge Point to change to a status it is already in, Charge Point SHALL
               respond with availability status ‘Accepted’. */
            // Currently there is no reason to not accept this.
            // If in the future a connector can transition to UNAVAILABLE on its own, check here if the reason for this transition was fixed.
            switch(state) {
                case OcppState::PowerOn:
                case OcppState::Pending:
                case OcppState::Rejected:
                case OcppState::Faulted:
                case OcppState::SoftReset:
                case OcppState::HardReset:
                    return ChangeAvailabilityResponseStatus::REJECTED;

                case OcppState::Idle:
                case OcppState::Unavailable:
                    this->state = OcppState::Idle;
                    return ChangeAvailabilityResponseStatus::ACCEPTED;

                case OcppState::FlushPersistentMessages:
                    this->state = OcppState::Idle;
                    return ChangeAvailabilityResponseStatus::ACCEPTED;
            }
            break;

        case ChangeAvailabilityType::INOPERATIVE:
             switch(state) {
                case OcppState::PowerOn:
                case OcppState::Pending:
                case OcppState::Rejected:
                case OcppState::Faulted:
                case OcppState::SoftReset:
                case OcppState::HardReset:
                case OcppState::FlushPersistentMessages:
                    return ChangeAvailabilityResponseStatus::REJECTED;

                case OcppState::Idle:
                case OcppState::Unavailable:
                    this->state = OcppState::Unavailable;
                    return ChangeAvailabilityResponseStatus::ACCEPTED;
            }
            break;
    }
}
// ("false" + ',') * (NUM_CONNECTORS + 1) + '[' + ']' + '\0'
#define AVAILABLE_STRING_BUF_SIZE 6 * (NUM_CONNECTORS + 1) + 3

void OcppChargePoint::saveAvailability()
{
    StaticJsonDocument<JSON_ARRAY_SIZE(NUM_CONNECTORS + 1)> doc;
    doc.add(this->state != OcppState::Unavailable);

    for(int i = 0; i < NUM_CONNECTORS; ++i) {
        doc.add(!connectors[i].willBeUnavailable());
    }

    auto buf = heap_alloc_array<char>(AVAILABLE_STRING_BUF_SIZE);

    size_t written = serializeJson(doc, buf.get(), AVAILABLE_STRING_BUF_SIZE);
    platform_write_file("avail", buf.get(), written);
}


void OcppChargePoint::loadAvailability()
{
    auto buf = heap_alloc_array<char>(AVAILABLE_STRING_BUF_SIZE);
    size_t len = platform_read_file("avail", buf.get(), AVAILABLE_STRING_BUF_SIZE);

    StaticJsonDocument<JSON_ARRAY_SIZE(NUM_CONNECTORS + 1)> doc;
    if (deserializeJson(doc, buf.get(), len) != DeserializationError::Ok)
        return;

    if (!doc.is<JsonArray>())
        return;

    for(int i = 0; i < NUM_CONNECTORS; ++i) {
        if (!doc[i + 1].as<bool>())
            connectors[i].onChangeAvailability(ChangeAvailabilityType::INOPERATIVE);
    }
}

StatusNotificationStatus OcppChargePoint::getStatus()
{
    switch(state) {
        case OcppState::Unavailable:
        case OcppState::SoftReset:
        case OcppState::HardReset:
            return StatusNotificationStatus::UNAVAILABLE;

        case OcppState::Faulted:
            return StatusNotificationStatus::FAULTED;

        case OcppState::PowerOn:
        case OcppState::Pending:
        case OcppState::Rejected:
        case OcppState::Idle:
        case OcppState::FlushPersistentMessages:
            return StatusNotificationStatus::AVAILABLE;
    }
}

void OcppChargePoint::sendStatus()
{
    StatusNotificationStatus newStatus = getStatus();
    if (last_sent_status == newStatus)
        return;

    forceSendStatus();
}

void OcppChargePoint::forceSendStatus()
{
    StatusNotificationStatus newStatus = getStatus();
    log_info("Sending StatusNotification.req: Status %s for charge point", StatusNotificationStatusStrings[(size_t)newStatus]);

    this->sendCallAction(StatusNotification(0, StatusNotificationErrorCode::NO_ERROR, newStatus, nullptr, platform_get_system_time(this->platform_ctx)));
    last_sent_status = newStatus;
}

bool OcppChargePoint::sendCallAction(const ICall &call, time_t timestamp, int32_t connectorId)
{
    switch (state) {
        case OcppState::PowerOn:
        case OcppState::Pending:
        case OcppState::Rejected:
            if (call.action != CallAction::BOOT_NOTIFICATION)
                return false;
            break;
        case OcppState::Idle:
        case OcppState::Unavailable:
        case OcppState::Faulted:
        case OcppState::SoftReset:
        case OcppState::HardReset:
        case OcppState::FlushPersistentMessages:
            break;
    }

    connection.sendCallAction(call, timestamp, connectorId);
    return true;
}

void OcppChargePoint::onTimeout(CallAction action, uint32_t messageId, int32_t connectorId)
{
    switch (action) {
        case CallAction::AUTHORIZE:
            if (connectorId > 0 && connectorId <= NUM_CONNECTORS)
                connectors[connectorId].onAuthorizeError();
            break;

        // Transaction related messages only trigger the timeout
        // if all attempts to resend them failed
        // because the central failed to process the message
        // -> Nothing to do here
        case CallAction::METER_VALUES:
        case CallAction::START_TRANSACTION:
        case CallAction::STOP_TRANSACTION:

        // Nothing to do here
        case CallAction::BOOT_NOTIFICATION:
        case CallAction::HEARTBEAT:
        case CallAction::STATUS_NOTIFICATION:

        // Not core profile - should be unreachable
        case CallAction::CANCEL_RESERVATION:
        case CallAction::CLEAR_CACHE:
        case CallAction::CLEAR_CHARGING_PROFILE:
        case CallAction::DATA_TRANSFER:
        case CallAction::DIAGNOSTICS_STATUS_NOTIFICATION:
        case CallAction::FIRMWARE_STATUS_NOTIFICATION:
        case CallAction::GET_COMPOSITE_SCHEDULE:
        case CallAction::GET_DIAGNOSTICS:
        case CallAction::GET_LOCAL_LIST_VERSION:
        case CallAction::RESERVE_NOW:
        case CallAction::SEND_LOCAL_LIST:
        case CallAction::SET_CHARGING_PROFILE:
        case CallAction::TRIGGER_MESSAGE:
        case CallAction::UPDATE_FIRMWARE:

        // Requests that are only sent from the central to us - should be unreachable
        case CallAction::CHANGE_AVAILABILITY:
        case CallAction::CHANGE_CONFIGURATION:
        case CallAction::GET_CONFIGURATION:
        case CallAction::REMOTE_START_TRANSACTION:
        case CallAction::REMOTE_STOP_TRANSACTION:
        case CallAction::RESET:
        case CallAction::UNLOCK_CONNECTOR:

        // Responses - should be unreachable
        case CallAction::CHANGE_AVAILABILITY_RESPONSE:
        case CallAction::CHANGE_CONFIGURATION_RESPONSE:
        case CallAction::CLEAR_CACHE_RESPONSE:
        case CallAction::DATA_TRANSFER_RESPONSE:
        case CallAction::GET_CONFIGURATION_RESPONSE:
        case CallAction::REMOTE_START_TRANSACTION_RESPONSE:
        case CallAction::REMOTE_STOP_TRANSACTION_RESPONSE:
        case CallAction::RESET_RESPONSE:
        case CallAction::UNLOCK_CONNECTOR_RESPONSE:
        case CallAction::AUTHORIZE_RESPONSE:
        case CallAction::BOOT_NOTIFICATION_RESPONSE:
        case CallAction::HEARTBEAT_RESPONSE:
        case CallAction::METER_VALUES_RESPONSE:
        case CallAction::START_TRANSACTION_RESPONSE:
        case CallAction::STATUS_NOTIFICATION_RESPONSE:
        case CallAction::STOP_TRANSACTION_RESPONSE:
        case CallAction::GET_DIAGNOSTICS_RESPONSE:
        case CallAction::UPDATE_FIRMWARE_RESPONSE:
        case CallAction::DIAGNOSTICS_STATUS_NOTIFICATION_RESPONSE:
        case CallAction::FIRMWARE_STATUS_NOTIFICATION_RESPONSE:
        case CallAction::GET_LOCAL_LIST_VERSION_RESPONSE:
        case CallAction::SEND_LOCAL_LIST_RESPONSE:
        case CallAction::CANCEL_RESERVATION_RESPONSE:
        case CallAction::RESERVE_NOW_RESPONSE:
        case CallAction::CLEAR_CHARGING_PROFILE_RESPONSE:
        case CallAction::GET_COMPOSITE_SCHEDULE_RESPONSE:
        case CallAction::SET_CHARGING_PROFILE_RESPONSE:
        case CallAction::TRIGGER_MESSAGE_RESPONSE:
            break;
    }
}

CallResponse OcppChargePoint::handleAuthorizeResponse(int32_t connectorId, AuthorizeResponseView conf)
{
    log_info("Received Authorize.conf for connector %d\n", connectorId);
    IdTagInfo info;
    info.updateFromIdTagInfo(conf.idTagInfo());

    if (connectorId > 0 && connectorId <= NUM_CONNECTORS)
        connectors[connectorId - 1].onAuthorizeConf(info);

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleBootNotificationResponse(int32_t connectorId, BootNotificationResponseView conf) {
    (void) connectorId;
    log_info("Received BootNotification.conf for connector %d\n", connectorId);

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
        setIntConfig(ConfigKey::HeartbeatInterval, DEFAULT_HEARTBEAT_INTERVAL_S);

    switch (conf.status()) {
        case BootNotificationResponseStatus::ACCEPTED: {
            platform_set_system_time(platform_ctx, conf.currentTime());

            state = OcppState::Idle;

            this->forceSendStatus();

            for(size_t i = 0; i < NUM_CONNECTORS; ++i)
                connectors[i].forceSendStatus();

            if (startRestore())
                this->state = OcppState::FlushPersistentMessages;

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
    log_info("Received ChangeAvilability.req connector %d to %s", req.connectorId(), ChangeAvailabilityTypeStrings[(size_t)req.type()]);
    int conn_id = req.connectorId();
    auto resp = ChangeAvailabilityResponseStatus::NONE;

    if (conn_id == 0) {
        resp = this->onChangeAvailability(req.type());

        if (resp != ChangeAvailabilityResponseStatus::REJECTED) {
            for(size_t i = 0; i < NUM_CONNECTORS; ++i) {
                auto conn_resp = connectors[i].onChangeAvailability(req.type());
                if (conn_resp == ChangeAvailabilityResponseStatus::REJECTED)
                    log_warn("Connectors rejecting the change availability request is not supported yet!");
                else if (conn_resp == ChangeAvailabilityResponseStatus::SCHEDULED)
                    resp = conn_resp;
            }
        }
    } else if (conn_id < 0 || conn_id > NUM_CONNECTORS)
        resp = ChangeAvailabilityResponseStatus::REJECTED;
    else
        resp = connectors[conn_id - 1].onChangeAvailability(req.type());

    log_info("Sending ChangeAvailablility.conf %s\n", ChangeAvailabilityResponseStatusStrings[(size_t)resp]);
    connection.sendCallResponse(ChangeAvailabilityResponse(uid, resp));

    if (resp != ChangeAvailabilityResponseStatus::REJECTED)
        saveAvailability();

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleChangeConfiguration(const char *uid, ChangeConfigurationView req)
{
    log_info("Received ChangeConfiguration.req %s to %s", req.key(), req.value());
    size_t key_idx;
    ChangeConfigurationResponseStatus status = ChangeConfigurationResponseStatus::ACCEPTED;
    if (!lookup_key(&key_idx, req.key(), config_keys, CONFIG_COUNT)) {
        status = ChangeConfigurationResponseStatus::NOT_SUPPORTED;
    } else if (getConfig(key_idx).readonly) {
        status = ChangeConfigurationResponseStatus::REJECTED;
    } else {
        status = getConfig(key_idx).setValue(req.value());
    }

    if (status == ChangeConfigurationResponseStatus::ACCEPTED || status == ChangeConfigurationResponseStatus::REBOOT_REQUIRED)
        saveConfig();

    log_info("Sending ChangeConfiguration.conf %s\n", ChangeConfigurationResponseStatusStrings[(size_t)status]);
    connection.sendCallResponse(ChangeConfigurationResponse(uid, status));

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleClearCache(const char *uid, ClearCacheView req)
{
    log_info("Received ClearCache.req");
    (void) req;
    /* Errata 4.0 3.23:
    In OCPP 1.6, the Cache is not required, but the message: ClearCache.req is required to be implemented. OCPP
    does not define what the expected behaviour is.
    Additional text
    When the Authorization Cache is not implemented and the Charge Point receives a ClearCache.req message. The Charge Point
    SHALL response with ClearCache.conf with the status: Rejected.
    */
    log_info("Sending ClearCache.conf Rejected\n");
    connection.sendCallResponse(ClearCacheResponse(uid, ResponseStatus::REJECTED));

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleDataTransfer(const char *uid, DataTransferView req)
{
    log_info("Received DataTransfer.req vendorId %s messageId %s", req.vendorId(), req.messageId().is_set() ? req.messageId().get() : "[NOT SET]");
    (void) req;
    /*
    If the recipient of the request has no implementation for the specific vendorId it SHALL return a status
    ‘UnknownVendor’ and the data element SHALL not be present.
    */
    log_info("Sending DataTransfer.conf UnknownVendorId\n");
    connection.sendCallResponse(DataTransferResponse(uid, DataTransferResponseStatus::UNKNOWN_VENDOR_ID));

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleDataTransferResponse(int32_t connectorId, DataTransferResponseView conf)
{
    log_info("Received DataTransfer.conf for connector %d. Status %s\n", connectorId, DataTransferResponseStatusStrings[(size_t)conf.status()]);
    (void) connectorId;
    (void) conf;

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleGetConfiguration(const char *uid, GetConfigurationView req)
{
    log_info("Received GetConfiguration.req with %lu keys", req.key_count());
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

    auto known = heap_alloc_array<GetConfigurationResponseConfigurationKey>(known_keys);
    size_t known_idx = 0;

    auto unknown = heap_alloc_array<const char *>(unknown_keys);
    size_t unknown_idx = 0;

    // Scratch buffer used for strings created from int config values.
    auto scratch_buf = heap_alloc_array<char>(scratch_buf_size);
    size_t scratch_buf_idx = 0;

    if (dump_all) {
        for(size_t i = 0; i < known_keys; ++i) {
            const char *config_value;
            OcppConfiguration &config = getConfig(i);
            switch(config.type) {
                case OcppConfigurationValueType::Boolean:
                    config_value = config.value.boolean.b ? "true" : "false";
                    break;
                case OcppConfigurationValueType::Integer: {
                        config_value = (const char *)&scratch_buf[scratch_buf_idx];
                        int written = snprintf(&scratch_buf[scratch_buf_idx], scratch_buf_size - scratch_buf_idx, "%d", config.value.integer.i);
                        if (written < 0) {
                            log_error("Failed to dump all configuration: %d", written);
                            break; //TODO: what to do if this happens?
                        }
                        scratch_buf_idx += (size_t)written;
                        ++scratch_buf_idx; // for null terminator
                    }
                    break;
                case OcppConfigurationValueType::CSL:
                    config_value = config.value.csl.c;
                    break;
            }
            known[known_idx].key = config_keys[i];
            known[known_idx].readonly = config.readonly;
            known[known_idx].value = config_value;
            ++known_idx;
            log_info("    %s: %s", config_keys[i], config_value);
        }
    } else {
        for(size_t i = 0; i < req.key_count(); ++i) {
            size_t result = i;
            if (lookup_key(&result, req.key(i).get(), config_keys, ARRAY_SIZE(config_keys))) {
                const char *config_value = "";
                OcppConfiguration &config = getConfig(result);
                switch(config.type) {
                    case OcppConfigurationValueType::Boolean:
                        config_value = config.value.boolean.b ? "true" : "false";
                        break;
                    case OcppConfigurationValueType::Integer: {
                            config_value = (const char *)&scratch_buf[scratch_buf_idx];
                            int written = snprintf(&scratch_buf[scratch_buf_idx], scratch_buf_size - scratch_buf_idx, "%d", config.value.integer.i);
                            if (written < 0) {
                                log_error("Failed to write int config value: %d", written);
                                continue; //TODO: what to do if this happens?
                            }
                            scratch_buf_idx += (size_t)written;
                            ++scratch_buf_idx; // for null terminator
                        }
                        break;
                    case OcppConfigurationValueType::CSL:
                        config_value = config.value.csl.c;
                        break;
                }
                known[known_idx].key = config_keys[result];
                known[known_idx].readonly = config.readonly;
                known[known_idx].value = config_value;
                log_info("    %s: %s", config_keys[result], config_value);
                ++known_idx;
            }
            else {
                unknown[unknown_idx] = req.key(i).get();
                ++unknown_idx;
                log_info("    %s: [UNKNOWN KEY]", req.key(i).get());
            }
        }
    }

    log_info("Sending GetConfiguration.conf with %lu known and %lu unknown keys\n", known_keys, unknown_keys);
    connection.sendCallResponse(GetConfigurationResponse(uid, known.get(), known_keys, unknown.get(), unknown_keys));

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleHeartbeatResponse(int32_t connectorId, HeartbeatResponseView conf)
{
    log_info("Received Heartbeat.conf for connector %d\n", connectorId);
    (void)connectorId;
    platform_set_system_time(platform_ctx, conf.currentTime());
    return CallResponse{CallErrorCode::OK, ""};

}

CallResponse OcppChargePoint::handleMeterValuesResponse(int32_t connectorId, MeterValuesResponseView conf)
{
    log_info("Received MeterValues.conf for connector %d\n", connectorId);
    (void) connectorId;
    (void) conf;
    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleRemoteStartTransaction(const char *uid, RemoteStartTransactionView req)
{
    log_info("Received RemoteStartTransaction.req for connector %d%s and tag %s", req.connectorId().is_set() ? req.connectorId() : 0, req.connectorId().is_set() ? "" : "[ANY CONNECTOR]", req.idTag());
    int conn_id = -1;

    if (!req.connectorId().is_set()) {
        for(int i = 0; i < NUM_CONNECTORS; ++i) {
            if (!connectors[i].isSelectableForRemoteStartTxn())
                continue;

            if (conn_id != -1) {
                // We already have another selectable connector.
                // -> Reject the request as it is ambiguous which connector to start.
                log_info("Sending RemoteStartTransaction.conf Rejected (ambiguous)\n");
                connection.sendCallResponse(RemoteStartTransactionResponse(uid, ResponseStatus::REJECTED));
                return CallResponse{CallErrorCode::OK, ""};
            }

            conn_id = i;
        }
    } else
        conn_id = req.connectorId().get() - 1;

    if (conn_id < 0 || conn_id >= NUM_CONNECTORS) {
        log_info("Sending RemoteStartTransaction.conf Rejected (unknown connector id)\n");
        connection.sendCallResponse(RemoteStartTransactionResponse(uid, ResponseStatus::REJECTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    // TODO: We could also reject here if the selected connector is faulted, unavailable or in an transaction.
    // However this is a different criterion than the one used in isSelectableForRemoteStartTxn,
    // as this function also requires at least the plug to return true. This is not required here, as we
    // can handle the request just as if the user starts interacting with the connector with a tag (i.e.
    // the IDLE -> AUTH_START_NO_PLUG transistion)

    log_info("Sending RemoteStartTransaction.conf Accepted\n");
    connection.sendCallResponse(RemoteStartTransactionResponse(uid, ResponseStatus::ACCEPTED));

    if (getBoolConfig(ConfigKey::AuthorizeRemoteTxRequests)) {
        connectors[conn_id].onTagSeen(req.idTag());
    } else {
        connectors[conn_id].onRemoteStartTransaction(req.idTag());
    }

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleRemoteStopTransaction(const char *uid, RemoteStopTransactionView req)
{
    log_info("Received RemoteStopTransaction.req");
    for(int i = 0; i < NUM_CONNECTORS; ++i) {
        if (!connectors[i].canHandleRemoteStopTxn(req.transactionId()))
            continue;

        connectors[i].onRemoteStopTransaction();
        log_info("Sending RemoteStopTransaction.conf Accepted\n");
        connection.sendCallResponse(RemoteStopTransactionResponse(uid, ResponseStatus::ACCEPTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    log_info("Sending RemoteStopTransaction.conf Rejected (unknown connector id)\n");
    connection.sendCallResponse(RemoteStopTransactionResponse(uid, ResponseStatus::REJECTED));
    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleReset(const char *uid, ResetView req)
{
    log_info("Received Reset.req");
    (void) uid;
    (void) req;

    log_info("Sending Request.conf Accepted\n");
    connection.sendCallResponse(ResetResponse(uid, ResponseStatus::ACCEPTED));

    for(size_t i = 0; i < NUM_CONNECTORS; ++i) {
        connectors[i].onRemoteStopTransaction();
        connectors[i].onChangeAvailability(ChangeAvailabilityType::INOPERATIVE);
    }

    if (req.type() == ResetType::SOFT) {
        this->state = OcppState::SoftReset;
    } else {
        this->state = OcppState::HardReset;
    }

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleStartTransactionResponse(int32_t connectorId, StartTransactionResponseView conf)
{
    log_info("Received StartTransaction.conf for connector %d\n", connectorId);
    IdTagInfo info;
    info.updateFromIdTagInfo(conf.idTagInfo());

    if (connectorId > 0 && connectorId <= NUM_CONNECTORS)
        connectors[connectorId - 1].onStartTransactionConf(info, conf.transactionId());

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleStatusNotificationResponse(int32_t connectorId, StatusNotificationResponseView conf)
{
    log_info("Received StatusNotification.conf for connector %d\n", connectorId);
    (void) connectorId;
    (void) conf;

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleStopTransactionResponse(int32_t connectorId, StopTransactionResponseView conf)
{
    log_info("Received StopTransaction.conf for connector %d\n", connectorId);
    (void) connectorId;
    (void) conf;

    // conf only contains a idTagInfo for updating the authorization cache. We don't implement that yet.
    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleUnlockConnector(const char *uid, UnlockConnectorView req)
{
    log_info("Received UnlockConnector.req");
    int32_t conn_id = req.connectorId() - 1;

    if (conn_id < 0 || conn_id >= NUM_CONNECTORS) {
        connection.sendCallResponse(UnlockConnectorResponse(uid, UnlockConnectorResponseStatus::NOT_SUPPORTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    auto result = connectors[conn_id].onUnlockConnector();

    log_info("Sending UnlockConnector.conf %s\n", UnlockConnectorResponseStatusStrings[(size_t)result]);
    connection.sendCallResponse(UnlockConnectorResponse(uid, result));
    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleClearChargingProfile(const char *uid, ClearChargingProfileView req)
{
    log_info("Received ClearChargingProfile.req");
    return CallResponse{CallErrorCode::NotImplemented, ""};
}

CallResponse OcppChargePoint::handleGetCompositeSchedule(const char *uid, GetCompositeScheduleView req)
{
    log_info("Received GetCompositeSchedule.req");
    return CallResponse{CallErrorCode::NotImplemented, ""};
}

CallResponse OcppChargePoint::handleSetChargingProfile(const char *uid, SetChargingProfileView req)
{
    log_info("Received SetChargingProfile.req");
    return CallResponse{CallErrorCode::NotImplemented, ""};
}

void OcppChargePoint::handleTagSeen(int32_t connectorId, const char *tagId)
{
    log_info("Seen tag %s at connector %d. State is %d", tagId, connectorId, (int)this->state);

    // Don't allow tags at connector 0 (i.e. the charge point itself)
    if (connectorId <= 0)
        return;

    if (connectorId > NUM_CONNECTORS)
        return;

    auto &conn = connectors[connectorId - 1];
    conn.onTagSeen(tagId);
}

void OcppChargePoint::handleStop(int32_t connectorId, StopReason reason) {
    log_info("connector %d wants to stop with reason %d", connectorId, (int)reason);

    // Don't allow tags at connector 0 (i.e. the charge point itself)
    if (connectorId <= 0)
        return;

    if (connectorId > NUM_CONNECTORS)
        return;

    auto &conn = connectors[connectorId - 1];
    conn.onStop(reason);
}

void OcppChargePoint::start(const char *websocket_endpoint_url, const char *charge_point_name_percent_encoded) {
    loadConfig();
    loadAvailability();
    platform_ctx = connection.start(websocket_endpoint_url, charge_point_name_percent_encoded, this);
    for(int32_t i = 0; i < NUM_CONNECTORS; ++i) {
        connectors[i].init(i + 1, this);
    }

    platform_register_tag_seen_callback(platform_ctx, [](int32_t connectorId, const char *tagId, void *user_data){((OcppChargePoint*)user_data)->handleTagSeen(connectorId, tagId);}, this);
    platform_register_stop_callback(platform_ctx, [](int32_t connectorId, StopReason reason, void *user_data){((OcppChargePoint*)user_data)->handleStop(connectorId, reason);}, this);
}
