#include "ChargePoint.h"

#include "Defines.h"
#include "Types.h"
#include "Configuration.h"
#include "Connector.h"
#include "Platform.h"

#include "Persistency.h"

#include <limits>
#include <array>
#include <algorithm>
#include <numeric>

extern "C" {
    #include "lib/libiso8601/iso8601.h"
}


//static StatusNotificationStatus last_connector_status[OCPP_NUM_CONNECTORS];

void OcppChargePoint::tick_power_on() {
    if (last_bn_send_ms == 0 && !platform_ws_connected(platform_ctx))
        return;

    uint32_t interval = getIntConfigUnsigned(ConfigKey::HeartbeatInterval);
    if (last_bn_send_ms != 0 && !deadline_elapsed(last_bn_send_ms + 1000 * interval))
        return;

    log_info("Sending BootNotification.req. %u %u %u %u", platform_now_ms(), last_bn_send_ms, last_bn_send_ms + 1000 * interval, 1000 * interval);

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

    uint32_t interval = getIntConfigUnsigned(ConfigKey::HeartbeatInterval);
    if (!deadline_elapsed(last_bn_send_ms + 1000 * interval))
        return;

    last_bn_send_ms = platform_now_ms();

    log_info("Sending Heartbeat.req. %u %u %u %u", platform_now_ms(), last_bn_send_ms, last_bn_send_ms + 1000 * interval, 1000 * interval);

    this->sendCallAction(Heartbeat());
}

void OcppChargePoint::tick_hard_reset() {
    platform_reset(true);
}

void OcppChargePoint::tick_soft_reset() {
    // Stay in soft reset until all transaction messages are sent (to the platforms websocket impl).
    // Then stop, but allow the platform as long as it takes to send those websocket messages.
    // Then reset.

    if (connection.transaction_messages.size() != 0)
        return;

    this->stop();

    platform_reset(false);
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
        for(int32_t i = 0; i < OCPP_NUM_CONNECTORS; ++i) {
            connectors[i].tick();
        }
    }
    connection.tick();

    if (platform_get_system_time(this->platform_ctx) >= next_profile_eval) {
        evalAndApplyChargingProfiles();
    }
#ifdef OCPP_STATE_CALLBACKS
    platform_update_chargepoint_state(state, last_sent_status, next_profile_eval);
#endif
}

void OcppChargePoint::onConnect()
{
    if (state != OcppState::Idle)
        return;

    this->forceSendStatus();
    for(int32_t i = 0; i < OCPP_NUM_CONNECTORS; ++i)
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
// ("false" + ',') * (OCPP_NUM_CONNECTORS + 1) + '[' + ']' + '\0'
#define AVAILABLE_STRING_BUF_SIZE 6 * (OCPP_NUM_CONNECTORS + 1) + 3

void OcppChargePoint::saveAvailability()
{
    StaticJsonDocument<JSON_ARRAY_SIZE(OCPP_NUM_CONNECTORS + 1)> doc;
    doc.add(this->state != OcppState::Unavailable);

    for(int32_t i = 0; i < OCPP_NUM_CONNECTORS; ++i) {
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

    StaticJsonDocument<JSON_ARRAY_SIZE(OCPP_NUM_CONNECTORS + 1)> doc;
    if (deserializeJson(doc, buf.get(), len) != DeserializationError::Ok)
        return;

    if (!doc.is<JsonArray>())
        return;

    for(int32_t i = 0; i < OCPP_NUM_CONNECTORS; ++i) {
        if (!doc[(size_t)(i + 1)].as<bool>())
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
    log_info("Sending StatusNotification.req: Status %s for connector 0", StatusNotificationStatusStrings[(size_t)newStatus]);

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

    if (call.action == CallAction::START_TRANSACTION || call.action == CallAction::STOP_TRANSACTION)
        this->triggerChargingProfileEval();

    /*
    If a transaction-specific profile with purpose TxProfile is present, it SHALL overrule the default charging profile
    with purpose TxDefaultProfile for the duration of the current transaction only. After the transaction is stopped,
    the profile SHOULD be deleted.
    */
    if (call.action == CallAction::STOP_TRANSACTION) {
        for(size_t stack_level = 0; stack_level <= OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL; ++stack_level) {
            removeChargingProfile(connectorId, ChargingProfilePurpose::TX_PROFILE, stack_level);
            connectors[connectorId - 1].txProfiles[stack_level].clear();
        }
    }

    return true;
}

void OcppChargePoint::onTimeout(CallAction action, uint64_t messageId, int32_t connectorId)
{
    (void) messageId;

    switch (action) {
        case CallAction::AUTHORIZE:
            if (connectorId > 0 && connectorId <= OCPP_NUM_CONNECTORS)
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

ChangeConfigurationResponseStatus OcppChargePoint::changeConfig(const char *key, const char *value)
{
    size_t key_idx;
    ChangeConfigurationResponseStatus status = ChangeConfigurationResponseStatus::ACCEPTED;
    if (!lookup_key(&key_idx, key, config_keys, OCPP_CONFIG_COUNT)) {
        return ChangeConfigurationResponseStatus::NOT_SUPPORTED;
    }

    if (getConfig(key_idx).readonly) {
        return ChangeConfigurationResponseStatus::REJECTED;
    }

    status = getConfig(key_idx).setValue(value);

    if (status == ChangeConfigurationResponseStatus::ACCEPTED || status == ChangeConfigurationResponseStatus::REBOOT_REQUIRED) {
#ifdef OCPP_STATE_CALLBACKS
        platform_update_config_state((ConfigKey) key_idx, value);
#endif
        saveConfig();
    }

    return status;
}

CallResponse OcppChargePoint::handleAuthorizeResponse(int32_t connectorId, AuthorizeResponseView conf)
{
    log_info("Received Authorize.conf for connector %d\n", connectorId);
    IdTagInfo info;
    info.updateFromIdTagInfo(conf.idTagInfo());

    if (connectorId > 0 && connectorId <= OCPP_NUM_CONNECTORS)
        connectors[connectorId - 1].onAuthorizeConf(info);

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleBootNotificationResponse(int32_t connectorId, BootNotificationResponseView conf) {
    (void) connectorId;
    log_info("Received BootNotification.conf for connector %d. Interval %d\n", connectorId, conf.interval());

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
        setIntConfig(ConfigKey::HeartbeatInterval, OCPP_DEFAULT_HEARTBEAT_INTERVAL_S);

    switch (conf.status()) {
        case BootNotificationResponseStatus::ACCEPTED: {
            platform_set_system_time(platform_ctx, conf.currentTime());

            state = OcppState::Idle;

            this->forceSendStatus();

            for(int32_t i = 0; i < OCPP_NUM_CONNECTORS; ++i)
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
            for(int32_t i = 0; i < OCPP_NUM_CONNECTORS; ++i) {
                auto conn_resp = connectors[i].onChangeAvailability(req.type());
                if (conn_resp == ChangeAvailabilityResponseStatus::REJECTED)
                    log_warn("Connectors rejecting the change availability request is not supported yet!");
                else if (conn_resp == ChangeAvailabilityResponseStatus::SCHEDULED)
                    resp = conn_resp;
            }
        }
    } else if (conn_id < 0 || conn_id > OCPP_NUM_CONNECTORS)
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

    ChangeConfigurationResponseStatus status = this->changeConfig(req.key(), req.value());

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
        known_keys = OCPP_CONFIG_COUNT;
        scratch_buf_size = 20 * known_keys;
        dump_all = true;
    } else {
        for(size_t i = 0; i < req.key_count(); ++i) {
            size_t result;
            if (lookup_key(&result, req.key(i).get(), config_keys, OCPP_CONFIG_COUNT)) {
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
                            log_error("Failed to dump all configuration: value of key %s: Error %d", config_keys[i], written);
                            return CallResponse{CallErrorCode::InternalError, "Failed to dump all configuration."};
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
                                log_error("Failed to write int config %s: %d", config_keys[result], written);
                                return CallResponse{CallErrorCode::InternalError, "Failed to write int config value."};
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

template<typename T>
static bool is_charging_profile_valid(T prof, int32_t conn_id) {
    if (prof.stackLevel() < 0 || prof.stackLevel() > OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL) {
        log_info("Rejected: stack level %d out of range", prof.stackLevel());
        return false;
    }

    if (prof.chargingSchedule().chargingSchedulePeriod_count() > OCPP_CHARGING_SCHEDULE_MAX_PERIODS)  {
        log_info("Rejected: charging schedule period count %u out of range", prof.chargingSchedule().chargingSchedulePeriod_count());
        return false;
    }

    //TODO check OCPP_CHARGING_SCHEDULE_ALLOWED_CHARGING_RATE_UNIT

    auto purpose = prof.chargingProfilePurpose();
    /* ChargePointMaxProfile can only be set at Charge Point ConnectorId 0.*/
    if (purpose == ChargingProfilePurpose::CHARGE_POINT_MAX_PROFILE && conn_id != 0) {
        log_info("Rejected: CHARGE_POINT_MAX_PROFILE for connector id %d != 0 not allowed", conn_id);
        return false;
    }

    /* TxProfile SHALL only be set at Charge Point ConnectorId >0. */
    if (purpose == ChargingProfilePurpose::TX_PROFILE && conn_id == 0) {
        log_info("Rejected: TX_PROFILE for connector id == 0 not allowed");
        return false;
    }

    // reject if either recurKind is set and kind is not recurring or if it is not set and kind is recurring.
    if (prof.chargingProfileKind() == ChargingProfileKind::RECURRING != prof.recurrencyKind().is_set()) {
        log_info("Rejected: RECURRING but recurrency set");
        return false;
    }

    return true;
}

CallResponse OcppChargePoint::handleRemoteStartTransaction(const char *uid, RemoteStartTransactionView req)
{
    log_info("Received RemoteStartTransaction.req for connector %d%s and tag %s", req.connectorId().is_set() ? req.connectorId().get() : 0, req.connectorId().is_set() ? "" : "[ANY CONNECTOR]", req.idTag());
    int conn_idx = -1;

    if (!req.connectorId().is_set()) {
        for(int32_t i = 0; i < OCPP_NUM_CONNECTORS; ++i) {
            if (!connectors[i].isSelectableForRemoteStartTxn())
                continue;

            if (conn_idx != -1) {
                // We already have another selectable connector.
                // -> Reject the request as it is ambiguous which connector to start.
                log_info("Sending RemoteStartTransaction.conf Rejected (ambiguous)\n");
                connection.sendCallResponse(RemoteStartTransactionResponse(uid, ResponseStatus::REJECTED));
                return CallResponse{CallErrorCode::OK, ""};
            }

            conn_idx = i;
        }
    } else
        conn_idx = req.connectorId().get() - 1;

    if (conn_idx < 0 || conn_idx >= OCPP_NUM_CONNECTORS) {
        log_info("Sending RemoteStartTransaction.conf Rejected (unknown connector id)\n");
        connection.sendCallResponse(RemoteStartTransactionResponse(uid, ResponseStatus::REJECTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    // The spec does not explicitly require this, however OCTT as a test for this.
    if (!connectors[conn_idx].canHandleRemoteStartTxn()) {
        log_info("Sending RemoteStartTransaction.conf Rejected (connector faulted, unavailable or already in transaction)\n");
        connection.sendCallResponse(RemoteStartTransactionResponse(uid, ResponseStatus::REJECTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    if (req.chargingProfile().is_set()) {
        log_info("RemoteStartTransaction.req contains charging profile");

        auto prof = req.chargingProfile().get();

        if (!is_charging_profile_valid(prof, conn_idx + 1)) {
            connection.sendCallResponse(RemoteStartTransactionResponse(uid, ResponseStatus::REJECTED));
            return CallResponse{CallErrorCode::OK, ""};
        }

        if(prof.chargingProfilePurpose() != ChargingProfilePurpose::TX_PROFILE)  {
            log_info("Rejected: Purpose of RemoteStartTransaction.req must be TxProfile; was %s", ChargingProfilePurposeStrings[(size_t)prof.chargingProfilePurpose()]);
            connection.sendCallResponse(RemoteStartTransactionResponse(uid, ResponseStatus::REJECTED));
            return CallResponse{CallErrorCode::OK, ""};
        }

        /*
        If the Central System includes a ChargingProfile, the ChargingProfilePurpose MUST be set to TxProfile and the
        transactionId SHALL NOT be set.
        */
        if(prof.transactionId().is_set())  {
            log_info("Rejected: Transaction ID of charging profile in RemoteStartTransaction.req shall not be set.");
            connection.sendCallResponse(RemoteStartTransactionResponse(uid, ResponseStatus::REJECTED));
            return CallResponse{CallErrorCode::OK, ""};
        }

        /*
        The Charge Point SHALL apply the given profile to the newly started transaction.
        */
        // but we don't have to do any special handling here:
        // TxDefaultProfiles are never used if we have a TxProfile of any stack level.
        // Also there should not be other TxProfiles, as those are cleared when stopping a transaction,
        // and are only accepted in RemoteStartTransaction.reqs and if there is a transaction already running.
        connectors[conn_idx].txProfiles[prof.stackLevel()] = {ChargingProfile(prof)};

        // This is probably not necessary, as there should not be another (older) TxProfile.
        removeChargingProfile(conn_idx + 1, ChargingProfilePurpose::TX_PROFILE, prof.stackLevel());

        persistChargingProfile(conn_idx + 1, &connectors[conn_idx].txProfiles[prof.stackLevel()].get());
    }

    // TODO: We could also reject here if the selected connector is faulted, unavailable or in an transaction.
    // However this is a different criterion than the one used in isSelectableForRemoteStartTxn,
    // as this function also requires at least the plug to return true. This is not required here, as we
    // can handle the request just as if the user starts interacting with the connector with a tag (i.e.
    // the IDLE -> AUTH_START_NO_PLUG transistion)

    log_info("Sending RemoteStartTransaction.conf Accepted\n");
    connection.sendCallResponse(RemoteStartTransactionResponse(uid, ResponseStatus::ACCEPTED));

    if (getBoolConfig(ConfigKey::AuthorizeRemoteTxRequests)) {
        connectors[conn_idx].onTagSeen(req.idTag());
    } else {
        connectors[conn_idx].onRemoteStartTransaction(req.idTag());
    }

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleRemoteStopTransaction(const char *uid, RemoteStopTransactionView req)
{
    log_info("Received RemoteStopTransaction.req for txn %d", req.transactionId());
    for(int32_t i = 0; i < OCPP_NUM_CONNECTORS; ++i) {
        if (!connectors[i].canHandleRemoteStopTxn(req.transactionId()))
            continue;

        connectors[i].onRemoteStopTransaction();
        log_info("Sending RemoteStopTransaction.conf Accepted (connector %d)\n", i);
        connection.sendCallResponse(RemoteStopTransactionResponse(uid, ResponseStatus::ACCEPTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    log_info("Sending RemoteStopTransaction.conf Rejected (unknown transaction id)\n");
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

    for(int32_t i = 0; i < OCPP_NUM_CONNECTORS; ++i) {
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

    if (connectorId > 0 && connectorId <= OCPP_NUM_CONNECTORS)
        connectors[connectorId - 1].onStartTransactionConf(info, conf.transactionId());

    // We received the txn-ID, so maybe now a TxProfile applies.
    this->triggerChargingProfileEval();

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

    auto result = UnlockConnectorResponseStatus::NONE;

    int32_t conn_id = req.connectorId() - 1;

    if (conn_id < 0 || conn_id >= OCPP_NUM_CONNECTORS)
        result = UnlockConnectorResponseStatus::NOT_SUPPORTED;
    else if (platform_has_fixed_cable(conn_id))
        result = UnlockConnectorResponseStatus::NOT_SUPPORTED;
    else
        result = connectors[conn_id].onUnlockConnector();

    log_info("Sending UnlockConnector.conf %s\n", UnlockConnectorResponseStatusStrings[(size_t)result]);
    connection.sendCallResponse(UnlockConnectorResponse(uid, result));
    return CallResponse{CallErrorCode::OK, ""};
}

static bool handleClearProfile(ClearChargingProfileView req, Opt<ChargingProfile> &opt, int connector_id) {
    if (!opt.is_set())
        return false;

    auto &prof = opt.get();

    /*
    (Errata 4.0) If id is specified, then all other fields are ignored.
    */
    if (req.id().is_set()) {
        if (req.id().get() == prof.id) {
            removeChargingProfile(connector_id, &prof);
            opt.clear();
            return true;
        }
        return false;
    }

    /*
    (Errata 4.0)
    The Central System can use this message to clear (remove) [...] a selection of
    charging profiles that match (logical AND) with the values of the optional connectorId, stackLevel and chargingProfilePurpose
    fields.
    If no fields are provided, then all charging profiles will be cleared.
    */

    bool clear = true;

    clear &= (!req.connectorId().is_set() || req.connectorId().get() == connector_id);
    clear &= (!req.stackLevel().is_set() || req.stackLevel().get() == prof.stackLevel);
    clear &= (!req.chargingProfilePurpose().is_set() || req.chargingProfilePurpose().get() == prof.chargingProfilePurpose);

    if (clear) {
        removeChargingProfile(connector_id, &prof);
        opt.clear();
    }

    return clear;
}

CallResponse OcppChargePoint::handleClearChargingProfile(const char *uid, ClearChargingProfileView req)
{
    log_info("Received ClearChargingProfile.req");

    bool accepted = false;

    for(size_t stack_level = 0; stack_level <= OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL; ++stack_level) {
        accepted |= handleClearProfile(req, this->chargePointMaxProfiles[stack_level], 0);
        accepted |= handleClearProfile(req, this->txDefaultProfiles[stack_level], 0);

        for(int32_t conn_id = 0; conn_id < OCPP_NUM_CONNECTORS; ++conn_id) {
            accepted |= handleClearProfile(req, connectors[conn_id].txProfiles[stack_level], conn_id + 1);
            accepted |= handleClearProfile(req, connectors[conn_id].txDefaultProfiles[stack_level], conn_id + 1);
        }
    }

    connection.sendCallResponse(ClearChargingProfileResponse(uid, accepted ? ClearChargingProfileResponseStatus::ACCEPTED : ClearChargingProfileResponseStatus::UNKNOWN));

    if (accepted)
        this->triggerChargingProfileEval();

    return CallResponse{CallErrorCode::OK, ""};
}

CallResponse OcppChargePoint::handleGetCompositeSchedule(const char *uid, GetCompositeScheduleView req)
{
    log_info("Received GetCompositeSchedule.req");

    int32_t conn_id = req.connectorId();
    if (conn_id < 0 || conn_id > OCPP_NUM_CONNECTORS) {
        connection.sendCallResponse(GetCompositeScheduleResponse(uid, ResponseStatus::REJECTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    GetCompositeScheduleResponseChargingSchedule sched;
    GetCompositeScheduleResponseChargingScheduleChargingSchedulePeriod periods[OCPP_GET_COMPOSITE_SCHEDULE_MAX_PERIODS];
    size_t periods_used = 0;

    time_t start = platform_get_system_time(platform_ctx);
    time_t next_check = start;
    time_t end = start + req.duration();

    while (periods_used < ARRAY_SIZE(periods) && next_check < end) {
        auto result = evalChargingProfiles(next_check);

        periods[periods_used].startPeriod = (int32_t) (next_check - start);

        next_check = result.nextCheck;

        periods[periods_used].numberPhases = result.allocatedPhases[req.connectorId()];

        periods[periods_used].limit = result.allocatedLimit[req.connectorId()];
        if (req.chargingRateUnit().is_set() && req.chargingRateUnit().get() == ChargingRateUnit::W)
            periods[periods_used].limit *= OCPP_LINE_VOLTAGE * periods[periods_used].numberPhases;

        if (periods_used > 0
         && periods[periods_used - 1].limit == periods[periods_used].limit
         && periods[periods_used - 1].numberPhases == periods[periods_used].numberPhases)
            // If this period and the last one have exactly the same limits, we don't have to report the second period.
            // This can happen, as evalChargingProfiles only returns the next time to check, not the next time a limit
            // change will happen in any case.
            continue;

        ++periods_used;
    }

    if (next_check < end) {
        connection.sendCallResponse(GetCompositeScheduleResponse(uid, ResponseStatus::REJECTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    sched.chargingSchedulePeriod = periods;
    sched.chargingSchedulePeriod_length = periods_used;
    sched.chargingRateUnit = (req.chargingRateUnit().is_set() && req.chargingRateUnit().get() == ChargingRateUnit::W) ?
                             GetCompositeScheduleResponseChargingScheduleChargingRateUnit::W :
                             GetCompositeScheduleResponseChargingScheduleChargingRateUnit::A;
    sched.duration = req.duration();

    /* Errata 4.0: When ChargingSchedule is used as part of a GetCompositeSchedule.conf message, then this field must be omitted. */
    //sched.startSchedule = start;

    connection.sendCallResponse(GetCompositeScheduleResponse(uid, ResponseStatus::ACCEPTED, req.connectorId(), start, &sched));
    return CallResponse{CallErrorCode::OK, ""};
}

static void clearProfileById(int32_t connectorId, int32_t id, Opt<ChargingProfile> *opt) {
    if (opt->is_set() && opt->get().id == id) {
        log_info("New profile replaces %s level %u", ChargingProfilePurposeStrings[(size_t)opt->get().chargingProfilePurpose], opt->get().stackLevel);
        removeChargingProfile(connectorId, &opt->get());
        opt->clear();
    }
}

CallResponse OcppChargePoint::handleSetChargingProfile(const char *uid, SetChargingProfileView req)
{
    log_info("Received SetChargingProfile.req stacklevel %d", req.csChargingProfiles().stackLevel());

    int32_t conn_id = req.connectorId();
    if (conn_id < 0 || conn_id > OCPP_NUM_CONNECTORS) {
        log_info("Rejected: connector ID %d out of range", conn_id);
        connection.sendCallResponse(SetChargingProfileResponse(uid, SetChargingProfileResponseStatus::REJECTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    auto prof = req.csChargingProfiles();

    if(!is_charging_profile_valid(prof, conn_id)) {
        connection.sendCallResponse(SetChargingProfileResponse(uid, SetChargingProfileResponseStatus::REJECTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    auto purpose = prof.chargingProfilePurpose();

    /* If there is no transaction active on the connector specified in a charging profile
       of type TxProfile, then the Charge Point SHALL discard it and return an error status in SetChargingProfile.conf. */
    if (purpose == ChargingProfilePurpose::TX_PROFILE && !connectors[conn_id - 1].isTransactionActive()) {
        log_info("Rejected: TX_PROFILE but no transaction active on connector %d", conn_id);
        connection.sendCallResponse(SetChargingProfileResponse(uid, SetChargingProfileResponseStatus::REJECTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    /* To prevent mismatch between transactions and a TxProfile, The Central System SHALL include
       the transactionId in a SetChargingProfile.req if the profile applies to a specific transaction. */
    if (purpose == ChargingProfilePurpose::TX_PROFILE && !prof.transactionId().is_set()) {
        log_info("Rejected: TX_PROFILE but no transaction id passed");
        connection.sendCallResponse(SetChargingProfileResponse(uid, SetChargingProfileResponseStatus::REJECTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    // The test tool expects us to reject here. The spec only implies this via the requirement to include a transaction ID.
    if (purpose == ChargingProfilePurpose::TX_PROFILE && connectors[conn_id - 1].transaction_id != prof.transactionId().get()) {
        log_info("Rejected: TX_PROFILE but no transaction active on connector %d", conn_id);
        connection.sendCallResponse(SetChargingProfileResponse(uid, SetChargingProfileResponseStatus::REJECTED));
        return CallResponse{CallErrorCode::OK, ""};
    }

    log_info("Charging profile accepted as %s level %d", ChargingProfilePurposeStrings[(size_t) purpose], prof.stackLevel());

    /*
    If a charging profile
    with the same chargingProfileId, or the same combination of stackLevel / ChargingProfilePurpose, exists on the
    Charge Point, the new charging profile SHALL replace the existing charging profile, otherwise it SHALL be added.
    */
    for(size_t stackLevel = 0; stackLevel < OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL; ++stackLevel) {
        clearProfileById(0, prof.chargingProfileId(), &this->chargePointMaxProfiles[stackLevel]);
        clearProfileById(0, prof.chargingProfileId(), &this->txDefaultProfiles[stackLevel]);

        for(size_t connectorIdx = 0; connectorIdx < OCPP_NUM_CONNECTORS; ++connectorIdx) {
            clearProfileById(connectorIdx + 1, prof.chargingProfileId(), &connectors[connectorIdx].txProfiles[stackLevel]);
            clearProfileById(connectorIdx + 1, prof.chargingProfileId(), &connectors[connectorIdx].txDefaultProfiles[stackLevel]);
        }
    }

    removeChargingProfile(conn_id, purpose, prof.stackLevel());

    Opt<ChargingProfile> *target = nullptr;

    if (conn_id == 0) {
        if (purpose == ChargingProfilePurpose::CHARGE_POINT_MAX_PROFILE)
            target = &this->chargePointMaxProfiles[prof.stackLevel()];
        else if (purpose == ChargingProfilePurpose::TX_DEFAULT_PROFILE)
            target = &this->txDefaultProfiles[prof.stackLevel()];
    } else {
        if (purpose == ChargingProfilePurpose::TX_PROFILE)
            target = &connectors[conn_id - 1].txProfiles[prof.stackLevel()];
        else if (purpose == ChargingProfilePurpose::TX_DEFAULT_PROFILE)
            target = &connectors[conn_id - 1].txDefaultProfiles[prof.stackLevel()];
    }

    if (target == nullptr) {
        log_error("Failed to find charging profile slot. This should never happen!");
        return CallResponse{CallErrorCode::InternalError, "Failed to find charging profile slot. This should never happen!"};
    }

    *target = {ChargingProfile(prof)};
    persistChargingProfile(conn_id, &target->get());

    log_info("Sending SetChargingProfile.conf.");
    connection.sendCallResponse(SetChargingProfileResponse(uid, SetChargingProfileResponseStatus::ACCEPTED));

    this->triggerChargingProfileEval();

    return CallResponse{CallErrorCode::OK, ""};
}

static void debug_print_limits(float *allowedLimit,
                        int32_t *allowedPhases,
                        float *minChargingRate,
                        bool trace) {
    if (trace)
        log_trace("    \tConnID\tAllowed\tPhases\tMinRate");
    else
        log_debug("    \tConnID\tAllowed\tPhases\tMinRate");
    for(int32_t i = 0; i < OCPP_NUM_CONNECTORS + 1; ++i) {
        if (trace)
            log_trace("    \t%d\t%.3f\t%d\t%.3f", i, allowedLimit[i], allowedPhases[i], minChargingRate[i]);
        else
            log_debug("    \t%d\t%.3f\t%d\t%.3f", i, allowedLimit[i], allowedPhases[i], minChargingRate[i]);
    }
}

OcppChargePoint::EvalChargingProfilesResult OcppChargePoint::evalChargingProfiles(time_t timeToEval)
{
    log_debug("Evaluating charging profiles");
    time_t nextCheck = std::numeric_limits<time_t>::max();

    // This is always a current. Charging profiles can contain current or power limits,
    // but power limits are converted to current limits when calling eval().
    float allowedCurrent[OCPP_NUM_CONNECTORS + 1];
    int32_t allowedPhases[OCPP_NUM_CONNECTORS + 1];
    float minChargingCurrent[OCPP_NUM_CONNECTORS + 1] = {0};

    for(int32_t i = 0; i < OCPP_NUM_CONNECTORS + 1; ++i) {
        allowedCurrent[i] = platform_get_maximum_charging_current(i) / 1000.0f;
        allowedPhases[i] = 3;
    }

    for(int stackLevel = OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL; stackLevel >= 0; --stackLevel) {
        if (!this->chargePointMaxProfiles[stackLevel].is_set()) {
            log_trace("    ChargePointMaxProfiles[%d] not set", stackLevel);
            continue;
        }

        auto result = this->chargePointMaxProfiles[stackLevel].get().eval(Opt<time_t>(), timeToEval);
        nextCheck = std::min(nextCheck, result.nextCheck);
        if (!result.applied) {
            log_trace("    ChargePointMaxProfiles[%d] did not apply", stackLevel);
            continue;
        }

        log_debug("    ChargePointMaxProfiles[%d] applied.", stackLevel);

        allowedCurrent[0] = std::min(allowedCurrent[0], result.currentLimit);
        allowedPhases[0] = std::min(allowedPhases[0], result.numberPhases);
        minChargingCurrent[0] = std::max(minChargingCurrent[0], result.minChargingCurrent);
        debug_print_limits(allowedCurrent, allowedPhases, minChargingCurrent, true);
        break;
    }

    for(int32_t connectorIdx = 0; connectorIdx < OCPP_NUM_CONNECTORS; ++connectorIdx) {
        auto &conn = connectors[connectorIdx];
        log_debug("    Connector %d", connectorIdx + 1);

        for(int stackLevel = OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL; stackLevel >= 0; --stackLevel) {
            if (!conn.txProfiles[stackLevel].is_set()) {
                log_trace("    TxProfiles[%d] not set", stackLevel);
                continue;
            }
            if (!conn.isTransactionActive()) {
                log_trace("    TxProfiles[%d] no txn running", stackLevel);
                continue;
            }

            if (conn.txProfiles[stackLevel].get().transactionId.is_set()) {
                auto connTxnId = conn.transaction_id;
                auto profTxnId = conn.txProfiles[stackLevel].get().transactionId.get();

                if (connTxnId != profTxnId){
                    log_trace("    TxProfiles[%d] txn ID %d != running txn ID %d", stackLevel, profTxnId, connTxnId);
                    continue;
                }
            }

            auto result = conn.txProfiles[stackLevel].get().eval(Opt<time_t>{conn.transaction_start_time}, timeToEval);

            nextCheck = std::min(nextCheck, result.nextCheck);
            if (!result.applied) {
                log_trace("    TxProfiles[%d] not applied", stackLevel);
                continue;
            }

            log_debug("    TxProfiles[%d] applied", stackLevel);

            allowedCurrent[connectorIdx + 1] = std::min(allowedCurrent[connectorIdx + 1], result.currentLimit);
            allowedPhases[connectorIdx + 1] = std::min(allowedPhases[connectorIdx + 1], result.numberPhases);
            minChargingCurrent[connectorIdx + 1] = std::max(minChargingCurrent[connectorIdx + 1], result.minChargingCurrent);
            debug_print_limits(allowedCurrent, allowedPhases, minChargingCurrent, true);
            log_trace("    A TxProfile applied. Skipping TxDefaultProfiles");
            goto profiles_evaluated;
        }

        // No TxProfile applied. Check TxDefaultProfiles.
        // no TxProfile applied, try TxDefaultProfiles
        // TxDefaultProfiles can be set for the whole charge point or only for one connector.
        // If a profile is set for a specific connector it overrides the one on the same stack level
        // of the whole charge point, but only for this connector.
        for(int stackLevel = OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL; stackLevel >= 0; --stackLevel) {
            ChargingProfile *prof = nullptr;
            if (conn.txDefaultProfiles[stackLevel].is_set()) {
                prof = &conn.txDefaultProfiles[stackLevel].get();
                log_trace("    TxDefaultProfiles[%d] set at connector", stackLevel);
            }
            else if (this->txDefaultProfiles[stackLevel].is_set()) {
                prof = &this->txDefaultProfiles[stackLevel].get();
                log_trace("    TxDefaultProfiles[%d] set at charge point", stackLevel);
            }

            if (prof == nullptr) {
                log_trace("    TxDefaultProfiles[%d] not set", stackLevel);
                continue;
            }

            Opt<time_t> txnTime;
            if (conn.isTransactionActive())
                txnTime = Opt<time_t>{conn.transaction_start_time};
            else
                txnTime = Opt<time_t>{};

            auto result = prof->eval(txnTime, timeToEval);

            nextCheck = std::min(nextCheck, result.nextCheck);
            if (!result.applied) {
                log_trace("    TxDefaultProfiles[%d] not applied", stackLevel);
                continue;
            }

            log_debug("    TxDefaultProfiles[%d] applied", stackLevel);

            allowedCurrent[connectorIdx + 1] = std::min(allowedCurrent[connectorIdx + 1], result.currentLimit);
            allowedPhases[connectorIdx + 1] = std::min(allowedPhases[connectorIdx + 1], result.numberPhases);
            minChargingCurrent[connectorIdx + 1] = std::max(minChargingCurrent[connectorIdx + 1], result.minChargingCurrent);
            debug_print_limits(allowedCurrent, allowedPhases, minChargingCurrent, true);
            goto profiles_evaluated;
        }
    }

profiles_evaluated:
    log_debug("    Profile evaluation done. Distributing limit");
    // We now know:
    // - The maximum allowed current and phases for the charge point (and thus all connectors)
    // - The maximum allowed current and phases for each connector
    // - The recommended minimum current for each connector

    auto availableLimit = allowedCurrent[0];

    EvalChargingProfilesResult result;
    result.nextCheck = nextCheck;
    result.allocatedLimit[0] = allowedCurrent[0];
    result.allocatedPhases[0] = allowedPhases[0];

    for(int32_t i = 1; i < OCPP_NUM_CONNECTORS + 1; ++i) {
        result.allocatedLimit[i] = 0;
        result.allocatedPhases[i] = 3;
    }


    for(int32_t connectorId = 1; connectorId < OCPP_NUM_CONNECTORS + 1; ++connectorId) {
        auto required = std::max(minChargingCurrent[0], std::max(minChargingCurrent[connectorId], (float)OCPP_CURRENT_REQUIRED_TO_START_CHARGING));
        auto allowed = std::min(availableLimit, allowedCurrent[connectorId]);
        if (required > allowed)
            break;

        result.allocatedLimit[connectorId] = required;
        availableLimit -= required;
    }

    // Now as many connectors as possible have the amount of current allocated that they need to (efficiently) charge.
    // Fairly redistribute the left-over current between those connectors.
    if (availableLimit > 0) {
        std::array<int32_t, OCPP_NUM_CONNECTORS> indices = {};
        std::iota(indices.begin(), indices.end(), 1);
        std::sort(indices.begin(), indices.end(), [allowedCurrent](int32_t a, int32_t b) {
            return allowedCurrent[a] > allowedCurrent[b];
        });

        for(size_t i = 0; i < indices.size(); ++i) {
            auto connId = indices[i];
            auto limit = std::min(availableLimit / (indices.size() - i), allowedCurrent[connId] - result.allocatedLimit[connId]);
            result.allocatedLimit[connId] += limit;
            availableLimit -= limit;
        }
    }

    log_debug("    Currents distributed:");
    debug_print_limits(result.allocatedLimit, result.allocatedPhases, minChargingCurrent, false);

    if (result.nextCheck < std::numeric_limits<time_t>::max()) {
        char buf[OCPP_ISO_8601_MAX_LEN] = {0};
        const tm *t = gmtime(&result.nextCheck);
        strftime(buf, ARRAY_SIZE(buf), "%FT%TZ", t);
        log_debug("    Next check: %s", buf);
    } else {
        log_debug("    Next check: never");
    }

    log_debug("");
    return result;
}

void OcppChargePoint::evalAndApplyChargingProfiles()
{
    auto result = evalChargingProfiles(platform_get_system_time(this->platform_ctx));
    this->next_profile_eval = std::min(result.nextCheck, platform_get_system_time(this->platform_ctx) + 5 * 60);

    for(int32_t i = 0; i < OCPP_NUM_CONNECTORS; ++i) {
        uint32_t limit = (uint32_t)(result.allocatedLimit[i + 1] * 1000);
        log_debug("Setting connector %d limit to %u", i + 1, limit);
        connectors[i].current_allowed = limit;
    }
}

void OcppChargePoint::triggerChargingProfileEval()
{
    this->next_profile_eval = platform_get_system_time(this->platform_ctx);
}

void OcppChargePoint::loadProfiles()
{
    for(size_t stackLevel = 0; stackLevel < OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL; ++stackLevel) {
        restoreChargingProfile(0, ChargingProfilePurpose::CHARGE_POINT_MAX_PROFILE, stackLevel, &this->chargePointMaxProfiles[stackLevel]);
        restoreChargingProfile(0, ChargingProfilePurpose::TX_DEFAULT_PROFILE, stackLevel, &this->chargePointMaxProfiles[stackLevel]);

        for(size_t connectorIdx = 0; connectorIdx < OCPP_NUM_CONNECTORS; ++connectorIdx) {
            restoreChargingProfile(connectorIdx + 1, ChargingProfilePurpose::TX_PROFILE, stackLevel, &connectors[connectorIdx].txProfiles[stackLevel]);
            restoreChargingProfile(connectorIdx + 1, ChargingProfilePurpose::TX_DEFAULT_PROFILE, stackLevel, &connectors[connectorIdx].txDefaultProfiles[stackLevel]);
        }
    }
}

void OcppChargePoint::handleTagSeen(int32_t connectorId, const char *tagId)
{
    log_info("Seen tag %s at connector %d. State is %d", tagId, connectorId, (int)this->state);

    // Don't allow tags at connector 0 (i.e. the charge point itself)
    if (connectorId <= 0)
        return;

    if (connectorId > OCPP_NUM_CONNECTORS)
        return;

    auto &conn = connectors[connectorId - 1];
    conn.onTagSeen(tagId);
}

void OcppChargePoint::handleStop(int32_t connectorId, StopReason reason) {
    log_info("connector %d wants to stop with reason %d", connectorId, (int)reason);

    // Don't allow tags at connector 0 (i.e. the charge point itself)
    if (connectorId <= 0)
        return;

    if (connectorId > OCPP_NUM_CONNECTORS)
        return;

    auto &conn = connectors[connectorId - 1];
    conn.onStop(reason);
}

static bool is_url_safe(char c) {
    // RFC 3986: unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           (c == '-') ||
           (c == '.') ||
           (c == '_') ||
           (c == '~');
}

static size_t url_encode(char *buf, size_t len, const char *url) {
    auto url_len = strlen(url);
    size_t offset = 0;
    const char * const hex = "0123456789abcdef";

    for(int i = 0; i < url_len; ++i) {
        // We want a logical right shift below, so use unsigned char here.
        unsigned char c = url[i];
        if (is_url_safe(c)) {
            if (offset < len)
                buf[offset] = c;
            ++offset;
        } else {
            if (offset < len) {
                buf[offset + 0] = '%';
                buf[offset + 1] = hex[c >> 4];
                buf[offset + 2] = hex[c & 0x0F];
            }
            offset += 3;
        }
    }

    if (offset < len)
        buf[offset] = '\0';
    else if (len > 0)
        buf[len - 1] = '\0';

    return offset;
}

void OcppChargePoint::start(const char *websocket_endpoint_url, const char *charge_point_name, const char *basic_auth_pass) {
    loadConfig();
#ifdef OCPP_STATE_CALLBACKS
    debugDumpConfig();
#endif
    loadAvailability();
    loadProfiles();

    auto encoded_len = url_encode(nullptr, 0, charge_point_name);

    auto buf = heap_alloc_array<char>(encoded_len + 1);

    url_encode(buf.get(), encoded_len + 1, charge_point_name);

    platform_ctx = connection.start(websocket_endpoint_url, buf.get(), charge_point_name, basic_auth_pass, this);
    for(int32_t i = 0; i < OCPP_NUM_CONNECTORS; ++i) {
        connectors[i].init(i + 1, this);
    }

    platform_register_tag_seen_callback(platform_ctx, [](int32_t connectorId, const char *tagId, void *user_data){((OcppChargePoint*)user_data)->handleTagSeen(connectorId, tagId);}, this);
    platform_register_stop_callback(platform_ctx, [](int32_t connectorId, StopReason reason, void *user_data){((OcppChargePoint*)user_data)->handleStop(connectorId, reason);}, this);
}

void OcppChargePoint::stop() {
    platform_disconnect(platform_ctx);
}
