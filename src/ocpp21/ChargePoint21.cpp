#include "ChargePoint21.h"

#include <string.h>

#include <common/Platform.h>
#include <common/Tools.h>

// TODO: derive from the platform/EVSE configuration when the transaction
// layer is implemented. A single EVSE with one connector is reported.
#define OCPP21_NUM_EVSES 1

#define OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S 30

namespace Ocpp21 {

bool ChargePoint21::start(const char *websocket_endpoint_url, const char *charge_point_name, const char *basic_auth_pass)
{
    if (connection.start(websocket_endpoint_url, charge_point_name, basic_auth_pass, this) == nullptr)
        return false;

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

void ChargePoint21::sendStatusNotifications()
{
    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
        // TODO: report the real connector state once the EVSE integration exists.
        connection.sendCallAction(StatusNotification{
            platform_get_system_time(connection.platform_ctx),
            StatusNotificationConnectorStatus::AVAILABLE,
            evse_id,
            1});
    }
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
    // TODO: OnIdle handling once transactions exist. For M1 every reset is immediate.
    (void)req;
    connection.sendCallResponse(ResetResponse{uid, ResetResponseStatus::ACCEPTED});
    // The response is sent from the connection tick before the platform
    // reset is allowed to tear down the process, see tick ordering. On the
    // Linux host platform_reset only logs.
    platform_reset(false);
    return CallResponse{CallErrorCode::OK, nullptr};
}

} // namespace Ocpp21
