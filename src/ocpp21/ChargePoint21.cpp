#include "ChargePoint21.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/Platform.h>
#include <common/Tools.h>

// TODO: derive from the platform/EVSE configuration. A single EVSE with
// one connector is enough for now.
#define OCPP21_NUM_EVSES 1

#define OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S 30

namespace Ocpp21 {

static void generate_transaction_id(char buf[OCPP21_TRANSACTION_ID_LEN + 1])
{
    // Pseudo UUIDv4. Not cryptographic, only collision avoidance across
    // reboots is needed.
    uint64_t r1 = ((uint64_t)rand() << 32) ^ (uint64_t)rand() ^ platform_now_ms();
    uint64_t r2 = ((uint64_t)rand() << 32) ^ (uint64_t)rand();
    snprintf(buf, OCPP21_TRANSACTION_ID_LEN + 1, "%08x-%04x-4%03x-8%03x-%012llx",
             (uint32_t)(r1 >> 32),
             (uint32_t)(r1 >> 16) & 0xffff,
             (uint32_t)r1 & 0xfff,
             (uint32_t)(r2 >> 48) & 0xfff,
             (unsigned long long)(r2 & 0xffffffffffffull));
}

bool ChargePoint21::start(const char *websocket_endpoint_url, const char *charge_point_name, const char *basic_auth_pass)
{
    if (connection.start(websocket_endpoint_url, charge_point_name, basic_auth_pass, this) == nullptr)
        return false;

    platform_register_tag_seen_callback21(connection.platform_ctx, [](int32_t evse_id, const char *tag_id, void *user_data){
        ((ChargePoint21*)user_data)->onTagSeen(evse_id, tag_id);
    }, this);

    boot_retry_deadline = set_deadline(0);
    return true;
}

void ChargePoint21::stop()
{
    connection.stop();
}

void ChargePoint21::tick()
{
    connection.tick();

    // Transactions continue while offline, their events are queued and
    // flushed after the reconnect.
    if (state == OcppState21::Idle)
        tickEvses();

    if (!platform_ws_connected(connection.platform_ctx))
        return;

    switch (state) {
        case OcppState21::PowerOn:
        case OcppState21::Pending:
        case OcppState21::Rejected:
            if ((deadline_elapsed(boot_retry_deadline) || trigger_boot_notification) && !boot_notification_in_flight) {
                trigger_boot_notification = false;
                sendBootNotification();
            }
            break;

        case OcppState21::Idle:
            if (status_notifications_pending || trigger_status_notification) {
                status_notifications_pending = false;
                trigger_status_notification = false;
                sendStatusNotifications();
            }

            if (deadline_elapsed(next_heartbeat_deadline) || trigger_heartbeat) {
                trigger_heartbeat = false;
                next_heartbeat_deadline = set_deadline((uint32_t)device_model.heartbeat_interval_s * 1000);
                connection.sendCallAction(Heartbeat{});
            }
            break;
    }
}

void ChargePoint21::onConnect()
{
    log_info("Connected (subprotocol ocpp2.1)");
    if (state != OcppState21::Idle)
        boot_retry_deadline = set_deadline(0);
    else
        status_notifications_pending = true;
}

void ChargePoint21::onDisconnect()
{
    log_info("Disconnected");
    boot_notification_in_flight = false;
}

void ChargePoint21::onTimeout(CallAction action, uint64_t messageId)
{
    (void)messageId;
    if (action == CallAction::BOOT_NOTIFICATION) {
        boot_notification_in_flight = false;
        boot_retry_deadline = set_deadline(OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S * 1000);
    }
    if (action == CallAction::AUTHORIZE) {
        log_info("Authorize timed out");
        authorize_in_flight = false;
    }
}

void ChargePoint21::onCallError(CallAction action, uint64_t messageId)
{
    this->onTimeout(action, messageId);
}

void ChargePoint21::sendBootNotification()
{
    BootNotificationChargingStation cs;
    cs.model = platform_get_charge_point_model();
    cs.vendorName = platform_get_charge_point_vendor();
    cs.serialNumber = platform_get_charge_point_serial_number();
    cs.firmwareVersion = platform_get_firmware_version();

    boot_notification_in_flight = connection.sendCallAction(BootNotification{&cs, BootNotificationReason::POWER_UP});
}

void ChargePoint21::onTagSeen(int32_t evse_id, const char *tag_id)
{
    if (tag_pending) {
        log_info("Tag %s seen while a tag is still being processed. Ignored", tag_id);
        return;
    }
    tag_evse_id = evse_id;
    strncpy(pending_tag, tag_id, OCPP21_ID_TOKEN_LEN);
    pending_tag[OCPP21_ID_TOKEN_LEN] = '\0';
    tag_pending = true;
}

static StatusNotificationConnectorStatus connector_status(EvseState21 s, const EvseTracker21 &t)
{
    if (s == EvseState21::Faulted)
        return StatusNotificationConnectorStatus::FAULTED;
    if (s != EvseState21::NotConnected || t.transaction_active || t.authorized)
        return StatusNotificationConnectorStatus::OCCUPIED;
    return StatusNotificationConnectorStatus::AVAILABLE;
}

void ChargePoint21::sendStatusNotifications()
{
    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
        auto &t = evses[evse_id - 1];
        auto status = connector_status(platform_get_evse_state21(connection.platform_ctx, evse_id), t);
        t.last_sent_status = status;
        connection.sendCallAction(StatusNotification{
            platform_get_system_time(connection.platform_ctx),
            status,
            evse_id,
            1});
    }
}

void ChargePoint21::tickEvses()
{
    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
        auto &t = evses[evse_id - 1];
        EvseState21 s = platform_get_evse_state21(connection.platform_ctx, evse_id);
        bool plugged = s == EvseState21::Connected || s == EvseState21::Charging;

        if (tag_pending && (tag_evse_id == 0 || tag_evse_id == evse_id)) {
            tag_pending = false;
            if (t.transaction_active) {
                if (strcmp(pending_tag, t.id_token) == 0)
                    stopTransaction(evse_id, TransactionEventTriggerReason::STOP_AUTHORIZED, TransactionEventTransactionInfoStoppedReason::LOCAL, true);
                else
                    log_info("Tag %s does not match the token of the running transaction. Ignored", pending_tag);
            } else if (t.authorized) {
                if (strcmp(pending_tag, t.id_token) == 0) {
                    log_info("Tag %s seen again before the EV was connected. Canceling the authorization", pending_tag);
                    t.authorized = false;
                    t.remote_start = false;
                } else {
                    log_info("Tag %s seen while another token is authorized. Ignored", pending_tag);
                }
            } else if (!authorize_in_flight) {
                if (!platform_ws_connected(connection.platform_ctx)) {
                    // TODO: offline authorization (local auth list or cache)
                    // is a later work package.
                    log_info("Tag %s seen while offline. Ignored", pending_tag);
                } else {
                    authorize_in_flight = true;
                    authorize_evse_id = evse_id;
                    strncpy(authorize_token, pending_tag, OCPP21_ID_TOKEN_LEN);
                    authorize_token[OCPP21_ID_TOKEN_LEN] = '\0';

                    AuthorizeIdToken token;
                    token.idToken = authorize_token;
                    token.type = "ISO14443";
                    connection.sendCallAction(Authorize{&token});
                }
            }
        }

        // The EV was not connected in time after the authorization.
        if (t.authorized && !t.transaction_active && !plugged && deadline_elapsed(t.ev_connect_deadline)) {
            log_info("EV connection timeout. Canceling the authorization for %s", t.id_token);
            t.authorized = false;
            t.remote_start = false;
        }

        // TxStartPoint PowerPathClosed, simplified: authorized and cable plugged.
        if (t.authorized && !t.transaction_active && plugged)
            startTransaction(evse_id, t.start_trigger);

        if (t.transaction_active) {
            if (s == EvseState21::NotConnected) {
                // TxCtrlr.StopTxOnEVSideDisconnect is fixed to true.
                stopTransaction(evse_id, TransactionEventTriggerReason::EV_COMMUNICATION_LOST, TransactionEventTransactionInfoStoppedReason::EV_DISCONNECTED, false);
            } else {
                auto cs = s == EvseState21::Charging ? TransactionEventTransactionInfoChargingState::CHARGING
                                                     : TransactionEventTransactionInfoChargingState::EV_CONNECTED;
                if (cs != t.charging_state) {
                    t.charging_state = cs;
                    sendTransactionUpdated(evse_id, TransactionEventTriggerReason::CHARGING_STATE_CHANGED, false);
                }

                if (device_model.tx_updated_interval_s > 0 && deadline_elapsed(t.next_sampled_value_deadline)) {
                    t.next_sampled_value_deadline = set_deadline((uint32_t)device_model.tx_updated_interval_s * 1000);
                    sendTransactionUpdated(evse_id, TransactionEventTriggerReason::METER_VALUE_PERIODIC, true);
                }
            }
        }

        auto status = connector_status(s, t);
        if (status != t.last_sent_status) {
            t.last_sent_status = status;
            connection.sendCallAction(StatusNotification{
                platform_get_system_time(connection.platform_ctx),
                status,
                evse_id,
                1});
        }

        t.last_state = s;
    }

    // A tag for an unknown EVSE id. Drop it.
    tag_pending = false;
}

void ChargePoint21::startTransaction(int32_t evse_id, TransactionEventTriggerReason trigger)
{
    auto &t = evses[evse_id - 1];

    generate_transaction_id(t.transaction_id);
    t.transaction_active = true;
    t.seq_no = 0;
    t.charging_state = TransactionEventTransactionInfoChargingState::EV_CONNECTED;

    log_info("Starting transaction %s on EVSE %d", t.transaction_id, evse_id);

    TransactionEventTransactionInfo info;
    info.transactionId = t.transaction_id;
    info.chargingState = t.charging_state;
    if (t.remote_start)
        info.remoteStartId = t.remote_start_id;

    TransactionEventMeterValueSampledValue sv;
    sv.value = platform_get_energy_wh21(connection.platform_ctx, evse_id);
    sv.measurand = MeterValueSampledValueMeasurand::ENERGY_ACTIVE_IMPORT_REGISTER;
    sv.context = MeterValueSampledValueContext::TRANSACTION_BEGIN;

    TransactionEventMeterValue mv;
    mv.sampledValue = &sv;
    mv.sampledValue_length = 1;
    mv.timestamp = platform_get_system_time(connection.platform_ctx);

    TransactionEventEvse evse;
    evse.id = evse_id;
    evse.connectorId = 1;

    TransactionEventIdToken token;
    token.idToken = t.id_token;
    token.type = t.id_token_type;

    connection.sendTransactionCallAction(TransactionEvent{
        TransactionEventEventType::STARTED,
        platform_get_system_time(connection.platform_ctx),
        trigger,
        t.seq_no++,
        &info,
        nullptr,
        &mv, 1,
        platform_ws_connected(connection.platform_ctx) ? (int8_t)OCPP_BOOL_NOT_PASSED : (int8_t)1,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        TransactionEventPreconditioningStatus::NONE,
        OCPP_BOOL_NOT_PASSED,
        &evse,
        &token});

    platform_set_charging_allowed21(connection.platform_ctx, evse_id, true);
    if (device_model.tx_updated_interval_s > 0)
        t.next_sampled_value_deadline = set_deadline((uint32_t)device_model.tx_updated_interval_s * 1000);
}

void ChargePoint21::sendTransactionUpdated(int32_t evse_id, TransactionEventTriggerReason trigger, bool with_meter_value)
{
    auto &t = evses[evse_id - 1];

    TransactionEventTransactionInfo info;
    info.transactionId = t.transaction_id;
    info.chargingState = t.charging_state;

    TransactionEventMeterValueSampledValue sv;
    sv.value = platform_get_energy_wh21(connection.platform_ctx, evse_id);
    sv.measurand = MeterValueSampledValueMeasurand::ENERGY_ACTIVE_IMPORT_REGISTER;
    sv.context = MeterValueSampledValueContext::SAMPLE_PERIODIC;

    TransactionEventMeterValue mv;
    mv.sampledValue = &sv;
    mv.sampledValue_length = 1;
    mv.timestamp = platform_get_system_time(connection.platform_ctx);

    TransactionEventEvse evse;
    evse.id = evse_id;
    evse.connectorId = 1;

    connection.sendTransactionCallAction(TransactionEvent{
        TransactionEventEventType::UPDATED,
        platform_get_system_time(connection.platform_ctx),
        trigger,
        t.seq_no++,
        &info,
        nullptr,
        with_meter_value ? &mv : nullptr, with_meter_value ? (size_t)1 : (size_t)0,
        platform_ws_connected(connection.platform_ctx) ? (int8_t)OCPP_BOOL_NOT_PASSED : (int8_t)1,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        TransactionEventPreconditioningStatus::NONE,
        OCPP_BOOL_NOT_PASSED,
        &evse});
}

void ChargePoint21::stopTransaction(int32_t evse_id, TransactionEventTriggerReason trigger, TransactionEventTransactionInfoStoppedReason reason, bool include_token)
{
    auto &t = evses[evse_id - 1];

    platform_set_charging_allowed21(connection.platform_ctx, evse_id, false);

    log_info("Stopping transaction %s on EVSE %d", t.transaction_id, evse_id);

    TransactionEventTransactionInfo info;
    info.transactionId = t.transaction_id;
    info.stoppedReason = reason;
    if (t.remote_start)
        info.remoteStartId = t.remote_start_id;

    TransactionEventMeterValueSampledValue sv;
    sv.value = platform_get_energy_wh21(connection.platform_ctx, evse_id);
    sv.measurand = MeterValueSampledValueMeasurand::ENERGY_ACTIVE_IMPORT_REGISTER;
    sv.context = MeterValueSampledValueContext::TRANSACTION_END;

    TransactionEventMeterValue mv;
    mv.sampledValue = &sv;
    mv.sampledValue_length = 1;
    mv.timestamp = platform_get_system_time(connection.platform_ctx);

    TransactionEventEvse evse;
    evse.id = evse_id;
    evse.connectorId = 1;

    TransactionEventIdToken token;
    token.idToken = t.id_token;
    token.type = t.id_token_type;

    connection.sendTransactionCallAction(TransactionEvent{
        TransactionEventEventType::ENDED,
        platform_get_system_time(connection.platform_ctx),
        trigger,
        t.seq_no++,
        &info,
        nullptr,
        &mv, 1,
        platform_ws_connected(connection.platform_ctx) ? (int8_t)OCPP_BOOL_NOT_PASSED : (int8_t)1,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        TransactionEventPreconditioningStatus::NONE,
        OCPP_BOOL_NOT_PASSED,
        &evse,
        include_token ? &token : nullptr});

    t.transaction_active = false;
    t.authorized = false;
    t.remote_start = false;
    t.charging_state = TransactionEventTransactionInfoChargingState::NONE;
    t.transaction_id[0] = '\0';
}

CallResponse ChargePoint21::handleBootNotificationResponse(int32_t connectorId, BootNotificationResponseView conf)
{
    (void)connectorId;
    boot_notification_in_flight = false;

    platform_set_system_time(connection.platform_ctx, conf.currentTime());

    int32_t interval = conf.interval();

    switch (conf.status()) {
        case BootNotificationResponseStatus::ACCEPTED:
            log_info("Boot notification accepted");
            state = OcppState21::Idle;
            if (interval > 0)
                device_model.heartbeat_interval_s = interval;
            next_heartbeat_deadline = set_deadline((uint32_t)device_model.heartbeat_interval_s * 1000);
            status_notifications_pending = true;
            break;

        case BootNotificationResponseStatus::PENDING:
            log_info("Boot notification pending, retrying in %d s", interval > 0 ? interval : OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S);
            state = OcppState21::Pending;
            boot_retry_deadline = set_deadline((uint32_t)(interval > 0 ? interval : OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S) * 1000);
            break;

        case BootNotificationResponseStatus::REJECTED:
            log_warn("Boot notification rejected, retrying in %d s", interval > 0 ? interval : OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S);
            state = OcppState21::Rejected;
            boot_retry_deadline = set_deadline((uint32_t)(interval > 0 ? interval : OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S) * 1000);
            break;
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleHeartbeatResponse(int32_t connectorId, HeartbeatResponseView conf)
{
    (void)connectorId;
    platform_set_system_time(connection.platform_ctx, conf.currentTime());
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleStatusNotificationResponse(int32_t connectorId, StatusNotificationResponseView conf)
{
    (void)connectorId;
    (void)conf;
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleGetVariables(const char *uid, GetVariablesView req)
{
    size_t count = req.getVariableData_count();
    if (count == 0)
        return CallResponse{CallErrorCode::OccurrenceConstraintViolation, "getVariableData must not be empty"};

    auto results = heap_alloc_array<GetVariablesResponseGetVariableResult>(count);
    auto components = heap_alloc_array<GetVariablesResponseGetVariableResultComponent>(count);
    auto variables = heap_alloc_array<GetVariablesResponseGetVariableResultVariable>(count);
    auto value_bufs = heap_alloc_array<char[64]>(count);

    for (size_t i = 0; i < count; ++i) {
        auto data = req.getVariableData(i);
        auto component = data.component();
        auto variable = data.variable();

        components[i].name = component.name();
        if (component.instance().is_some())
            components[i].instance = component.instance().unwrap();

        variables[i].name = variable.name();
        if (variable.instance().is_some())
            variables[i].instance = variable.instance().unwrap();

        results[i].component = &components[i];
        results[i].variable = &variables[i];

        if (data.attributeType().is_some())
            results[i].attributeType = (GetVariableResultAttributeEnumType)(size_t)data.attributeType().unwrap();

        // Only the Actual attribute is supported.
        if (data.attributeType().is_some() && data.attributeType().unwrap() != AttributeEnumType::ACTUAL) {
            results[i].attributeStatus = GetVariablesResponseGetVariableResultAttributeStatus::NOT_SUPPORTED_ATTRIBUTE_TYPE;
            continue;
        }

        auto res = device_model.getVariable(component.name(), variable.name(), value_bufs[i], sizeof(value_bufs[i]));
        switch (res) {
            case VariableResult::Accepted:
                results[i].attributeStatus = GetVariablesResponseGetVariableResultAttributeStatus::ACCEPTED;
                results[i].attributeValue = value_bufs[i];
                break;
            case VariableResult::UnknownComponent:
                results[i].attributeStatus = GetVariablesResponseGetVariableResultAttributeStatus::UNKNOWN_COMPONENT;
                break;
            case VariableResult::UnknownVariable:
                results[i].attributeStatus = GetVariablesResponseGetVariableResultAttributeStatus::UNKNOWN_VARIABLE;
                break;
            case VariableResult::Rejected:
            case VariableResult::NotSupportedAttributeType:
                results[i].attributeStatus = GetVariablesResponseGetVariableResultAttributeStatus::REJECTED;
                break;
        }
    }

    connection.sendCallResponse(GetVariablesResponse{uid, results.get(), count});
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleSetVariables(const char *uid, SetVariablesView req)
{
    size_t count = req.setVariableData_count();
    if (count == 0)
        return CallResponse{CallErrorCode::OccurrenceConstraintViolation, "setVariableData must not be empty"};

    auto results = heap_alloc_array<SetVariablesResponseSetVariableResult>(count);
    auto components = heap_alloc_array<SetVariablesResponseSetVariableResultComponent>(count);
    auto variables = heap_alloc_array<SetVariablesResponseSetVariableResultVariable>(count);

    for (size_t i = 0; i < count; ++i) {
        auto data = req.setVariableData(i);
        auto component = data.component();
        auto variable = data.variable();

        components[i].name = component.name();
        if (component.instance().is_some())
            components[i].instance = component.instance().unwrap();

        variables[i].name = variable.name();
        if (variable.instance().is_some())
            variables[i].instance = variable.instance().unwrap();

        results[i].component = &components[i];
        results[i].variable = &variables[i];

        if (data.attributeType().is_some())
            results[i].attributeType = (GetVariableResultAttributeEnumType)(size_t)data.attributeType().unwrap();

        if (data.attributeType().is_some() && data.attributeType().unwrap() != AttributeEnumType::ACTUAL) {
            results[i].attributeStatus = SetVariablesResponseSetVariableResultAttributeStatus::NOT_SUPPORTED_ATTRIBUTE_TYPE;
            continue;
        }

        auto res = device_model.setVariable(component.name(), variable.name(), data.attributeValue());
        switch (res) {
            case VariableResult::Accepted:
                results[i].attributeStatus = SetVariablesResponseSetVariableResultAttributeStatus::ACCEPTED;
                break;
            case VariableResult::UnknownComponent:
                results[i].attributeStatus = SetVariablesResponseSetVariableResultAttributeStatus::UNKNOWN_COMPONENT;
                break;
            case VariableResult::UnknownVariable:
                results[i].attributeStatus = SetVariablesResponseSetVariableResultAttributeStatus::UNKNOWN_VARIABLE;
                break;
            case VariableResult::Rejected:
            case VariableResult::NotSupportedAttributeType:
                results[i].attributeStatus = SetVariablesResponseSetVariableResultAttributeStatus::REJECTED;
                break;
        }
    }

    connection.sendCallResponse(SetVariablesResponse{uid, results.get(), count});
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleGetBaseReport(const char *uid, GetBaseReportView req)
{
    (void)req;
    // TODO: implement NotifyReport streaming for the base report.
    connection.sendCallResponse(GetBaseReportResponse{uid, GetBaseReportResponseStatus::NOT_SUPPORTED});
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleTriggerMessage(const char *uid, TriggerMessageView req)
{
    auto status = TriggerMessageResponseStatus::NOT_IMPLEMENTED;

    switch (req.requestedMessage()) {
        case TriggerMessageRequestedMessage::BOOT_NOTIFICATION:
            // B06: a triggered boot notification is only useful while not accepted.
            if (state == OcppState21::Idle) {
                status = TriggerMessageResponseStatus::REJECTED;
            } else {
                trigger_boot_notification = true;
                status = TriggerMessageResponseStatus::ACCEPTED;
            }
            break;

        case TriggerMessageRequestedMessage::HEARTBEAT:
            trigger_heartbeat = true;
            status = TriggerMessageResponseStatus::ACCEPTED;
            break;

        case TriggerMessageRequestedMessage::STATUS_NOTIFICATION:
            trigger_status_notification = true;
            status = TriggerMessageResponseStatus::ACCEPTED;
            break;

        default:
            status = TriggerMessageResponseStatus::NOT_IMPLEMENTED;
            break;
    }

    connection.sendCallResponse(TriggerMessageResponse{uid, status});
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleReset(const char *uid, ResetView req)
{
    // TODO: OnIdle handling once the transaction queue is persistent. Every
    // reset is treated as Immediate for now.
    (void)req;
    connection.sendCallResponse(ResetResponse{uid, ResetResponseStatus::ACCEPTED});

    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
        if (evses[evse_id - 1].transaction_active)
            stopTransaction(evse_id, TransactionEventTriggerReason::RESET_COMMAND, TransactionEventTransactionInfoStoppedReason::IMMEDIATE_RESET, false);
    }

    // The response is sent from the connection tick before the platform
    // reset is allowed to tear down the process, see tick ordering. On the
    // Linux host platform_reset only logs.
    platform_reset(false);
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleAuthorizeResponse(int32_t connectorId, AuthorizeResponseView conf)
{
    (void)connectorId;
    authorize_in_flight = false;

    if (authorize_evse_id < 1 || authorize_evse_id > OCPP21_NUM_EVSES)
        return CallResponse{CallErrorCode::OK, nullptr};

    auto &t = evses[authorize_evse_id - 1];
    auto status = conf.idTokenInfo().status();

    if (status != ResponseIdTokenInfoEntriesStatus::ACCEPTED) {
        log_info("Authorization of %s rejected (%s)", authorize_token, ResponseIdTokenInfoEntriesStatusStrings[(size_t)status]);
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    if (t.transaction_active || t.authorized) {
        log_info("Authorization of %s accepted, but the EVSE is no longer free. Ignored", authorize_token);
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    log_info("Authorization of %s accepted", authorize_token);

    memcpy(t.id_token, authorize_token, sizeof(t.id_token));
    strncpy(t.id_token_type, "ISO14443", OCPP21_ID_TOKEN_TYPE_LEN);
    t.id_token_type[OCPP21_ID_TOKEN_TYPE_LEN] = '\0';
    t.authorized = true;
    t.remote_start = false;

    auto s = platform_get_evse_state21(connection.platform_ctx, authorize_evse_id);
    bool plugged = s == EvseState21::Connected || s == EvseState21::Charging;
    // If the cable is already plugged the authorization completes the start
    // condition. Otherwise the plug event will.
    t.start_trigger = plugged ? TransactionEventTriggerReason::AUTHORIZED
                              : TransactionEventTriggerReason::CABLE_PLUGGED_IN;
    t.ev_connect_deadline = set_deadline((uint32_t)device_model.ev_connection_timeout_s * 1000);

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleTransactionEventResponse(int32_t connectorId, TransactionEventResponseView conf)
{
    (void)connectorId;

    // The response does not repeat the transaction id. As long as only one
    // EVSE exists the running transaction is unambiguous.
    auto idTokenInfo = conf.idTokenInfo();
    if (idTokenInfo.is_some() && idTokenInfo.unwrap().status() != ResponseIdTokenInfoEntriesStatus::ACCEPTED) {
        for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
            auto &t = evses[evse_id - 1];
            if (!t.transaction_active)
                continue;
            log_info("Transaction %s deauthorized by the CSMS", t.transaction_id);
            stopTransaction(evse_id, TransactionEventTriggerReason::DEAUTHORIZED, TransactionEventTransactionInfoStoppedReason::DE_AUTHORIZED, false);
        }
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleMeterValuesResponse(int32_t connectorId, MeterValuesResponseView conf)
{
    (void)connectorId;
    (void)conf;
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleRequestStartTransaction(const char *uid, RequestStartTransactionView req)
{
    int32_t evse_id = req.evseId().is_some() ? req.evseId().unwrap() : 1;

    if (evse_id < 1 || evse_id > OCPP21_NUM_EVSES) {
        connection.sendCallResponse(RequestStartTransactionResponse{uid, TransactionResponseStatus::REJECTED});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    auto &t = evses[evse_id - 1];
    if (t.transaction_active || t.authorized || authorize_in_flight) {
        connection.sendCallResponse(RequestStartTransactionResponse{uid, TransactionResponseStatus::REJECTED});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    auto idToken = req.idToken();
    strncpy(t.id_token, idToken.idToken(), OCPP21_ID_TOKEN_LEN);
    t.id_token[OCPP21_ID_TOKEN_LEN] = '\0';
    strncpy(t.id_token_type, idToken.type(), OCPP21_ID_TOKEN_TYPE_LEN);
    t.id_token_type[OCPP21_ID_TOKEN_TYPE_LEN] = '\0';

    // AuthCtrlr.AuthorizeRemoteStart is fixed to false: the token counts as
    // authorized without an Authorize round trip.
    t.authorized = true;
    t.remote_start = true;
    t.remote_start_id = req.remoteStartId();
    t.start_trigger = TransactionEventTriggerReason::REMOTE_START;
    t.ev_connect_deadline = set_deadline((uint32_t)device_model.ev_connection_timeout_s * 1000);

    if (req.chargingProfile().is_some())
        log_info("RequestStartTransaction contains a charging profile. Ignored, smart charging is not implemented yet");

    log_info("Remote start accepted for token %s on EVSE %d", t.id_token, evse_id);
    connection.sendCallResponse(RequestStartTransactionResponse{uid, TransactionResponseStatus::ACCEPTED});
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint21::handleRequestStopTransaction(const char *uid, RequestStopTransactionView req)
{
    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
        auto &t = evses[evse_id - 1];
        if (!t.transaction_active || strcmp(t.transaction_id, req.transactionId()) != 0)
            continue;

        log_info("Remote stop accepted for transaction %s", t.transaction_id);
        connection.sendCallResponse(RequestStopTransactionResponse{uid, TransactionResponseStatus::ACCEPTED});
        stopTransaction(evse_id, TransactionEventTriggerReason::REMOTE_STOP, TransactionEventTransactionInfoStoppedReason::REMOTE, false);
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    log_info("Remote stop rejected, unknown transaction %s", req.transactionId());
    connection.sendCallResponse(RequestStopTransactionResponse{uid, TransactionResponseStatus::REJECTED});
    return CallResponse{CallErrorCode::OK, nullptr};
}

} // namespace Ocpp21
