// THIS FILE IS AUTO-GENERATED. DO NOT EDIT.

#include "OcppMessages.h"

#include "OcppChargePoint.h"
#include "OcppPlatform.h"

extern "C" {
#include "lib/libiso8601/iso8601.h"
}

#include <TFJson.h>

static bool iso_string_to_unix_timestamp(const char *iso_string, time_t *t) {
    iso8601_time time;
    if (iso8601_parse(iso_string, &time) != 0) {
        return false;
    }
    iso8601_to_time_t(&time, t);
    return true;
}

static void unix_timestamp_to_iso_string(time_t timestamp, TFJsonSerializer &json, const char *key) {
    char buf[OCPP_ISO_8601_MAX_LEN] = {0};
    const tm *t = gmtime(&timestamp);

    strftime(buf, ARRAY_SIZE(buf), "%FT%TZ", t);

    json.add(key, buf);
}

ICall::~ICall() {}

size_t ICall::measureJson() const {
    return this->serializeJson(nullptr, 0);
}

static uint32_t next_call_id = 0;

const char * const ChangeAvailabilityResponseStatusStrings[] = {
    "Accepted",
    "Rejected",
    "Scheduled"
};

const char * const ChangeConfigurationResponseStatusStrings[] = {
    "Accepted",
    "Rejected",
    "RebootRequired",
    "NotSupported"
};

const char * const ResponseStatusStrings[] = {
    "Accepted",
    "Rejected"
};

const char * const DataTransferResponseStatusStrings[] = {
    "Accepted",
    "Rejected",
    "UnknownMessageId",
    "UnknownVendorId"
};

const char * const StatusNotificationErrorCodeStrings[] = {
    "ConnectorLockFailure",
    "EVCommunicationError",
    "GroundFailure",
    "HighTemperature",
    "InternalError",
    "LocalListConflict",
    "NoError",
    "OtherError",
    "OverCurrentFailure",
    "PowerMeterFailure",
    "PowerSwitchFailure",
    "ReaderFailure",
    "ResetFailure",
    "UnderVoltage",
    "OverVoltage",
    "WeakSignal"
};

const char * const StatusNotificationStatusStrings[] = {
    "Available",
    "Preparing",
    "Charging",
    "SuspendedEV",
    "SuspendedEVSE",
    "Finishing",
    "Reserved",
    "Unavailable",
    "Faulted"
};

const char * const StopTransactionReasonStrings[] = {
    "EmergencyStop",
    "EVDisconnected",
    "HardReset",
    "Local",
    "Other",
    "PowerLoss",
    "Reboot",
    "Remote",
    "SoftReset",
    "UnlockCommand",
    "DeAuthorized"
};

const char * const UnlockConnectorResponseStatusStrings[] = {
    "Unlocked",
    "UnlockFailed",
    "NotSupported"
};

const char * const ResponseIdTagInfoEntriesStatusStrings[] = {
    "Accepted",
    "Blocked",
    "Expired",
    "Invalid",
    "ConcurrentTx"
};

const char * const BootNotificationResponseStatusStrings[] = {
    "Accepted",
    "Pending",
    "Rejected"
};

const char * const ChangeAvailabilityTypeStrings[] = {
    "Inoperative",
    "Operative"
};

const char * const RemoteStartTransactionChargingProfileEntriesChargingProfilePurposeStrings[] = {
    "ChargePointMaxProfile",
    "TxDefaultProfile",
    "TxProfile"
};

const char * const RemoteStartTransactionChargingProfileEntriesChargingProfileKindStrings[] = {
    "Absolute",
    "Recurring",
    "Relative"
};

const char * const RemoteStartTransactionChargingProfileEntriesRecurrencyKindStrings[] = {
    "Daily",
    "Weekly"
};

const char * const RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingRateUnitStrings[] = {
    "A",
    "W"
};

const char * const ResetTypeStrings[] = {
    "Hard",
    "Soft"
};

const char * const SampledValueContextStrings[] = {
    "Interruption.Begin",
    "Interruption.End",
    "Sample.Clock",
    "Sample.Periodic",
    "Transaction.Begin",
    "Transaction.End",
    "Trigger",
    "Other"
};

const char * const SampledValueFormatStrings[] = {
    "Raw",
    "SignedData"
};

const char * const SampledValueMeasurandStrings[] = {
    "Energy.Active.Export.Register",
    "Energy.Active.Import.Register",
    "Energy.Reactive.Export.Register",
    "Energy.Reactive.Import.Register",
    "Energy.Active.Export.Interval",
    "Energy.Active.Import.Interval",
    "Energy.Reactive.Export.Interval",
    "Energy.Reactive.Import.Interval",
    "Power.Active.Export",
    "Power.Active.Import",
    "Power.Offered",
    "Power.Reactive.Export",
    "Power.Reactive.Import",
    "Power.Factor",
    "Current.Import",
    "Current.Export",
    "Current.Offered",
    "Voltage",
    "Frequency",
    "Temperature",
    "SoC",
    "RPM"
};

const char * const SampledValuePhaseStrings[] = {
    "L1",
    "L2",
    "L3",
    "N",
    "L1-N",
    "L2-N",
    "L3-N",
    "L1-L2",
    "L2-L3",
    "L3-L1"
};

const char * const SampledValueLocationStrings[] = {
    "Cable",
    "EV",
    "Inlet",
    "Outlet",
    "Body"
};

const char * const SampledValueUnitStrings[] = {
    "Wh",
    "kWh",
    "varh",
    "kvarh",
    "W",
    "kW",
    "VA",
    "kVA",
    "var",
    "kvar",
    "A",
    "V",
    "K",
    "Celsius",
    "Fahrenheit",
    "Percent"
};

const char * const CallActionStrings[] = {
    "Authorize",
    "BootNotification",
    "ChangeAvailabilityResponse",
    "ChangeConfigurationResponse",
    "ClearCacheResponse",
    "DataTransfer",
    "DataTransferResponse",
    "GetConfigurationResponse",
    "Heartbeat",
    "MeterValues",
    "RemoteStartTransactionResponse",
    "RemoteStopTransactionResponse",
    "ResetResponse",
    "StartTransaction",
    "StatusNotification",
    "StopTransaction",
    "UnlockConnectorResponse",
    "AuthorizeResponse",
    "BootNotificationResponse",
    "ChangeAvailability",
    "ChangeConfiguration",
    "ClearCache",
    "GetConfiguration",
    "HeartbeatResponse",
    "MeterValuesResponse",
    "RemoteStartTransaction",
    "RemoteStopTransaction",
    "Reset",
    "StartTransactionResponse",
    "StatusNotificationResponse",
    "StopTransactionResponse",
    "UnlockConnector",
    "GetDiagnosticsResponse",
    "DiagnosticsStatusNotification",
    "FirmwareStatusNotification",
    "UpdateFirmwareResponse",
    "GetDiagnostics",
    "DiagnosticsStatusNotificationResponse",
    "FirmwareStatusNotificationResponse",
    "UpdateFirmware",
    "GetLocalListVersionResponse",
    "SendLocalListResponse",
    "GetLocalListVersion",
    "SendLocalList",
    "CancelReservationResponse",
    "ReserveNowResponse",
    "CancelReservation",
    "ReserveNow",
    "ClearChargingProfileResponse",
    "GetCompositeScheduleResponse",
    "SetChargingProfileResponse",
    "ClearChargingProfile",
    "GetCompositeSchedule",
    "SetChargingProfile",
    "TriggerMessageResponse",
    "TriggerMessage"
};


void GetConfigurationResponseConfigurationKey::serializeInto(TFJsonSerializer &json) {
        if (key != nullptr) json.add("key", key);
        json.add("readonly", readonly);
        if (value != nullptr) json.add("value", value);
    }

void MeterValue::serializeInto(TFJsonSerializer &json) {
        if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, json, "timestamp");
        if (sampledValue != nullptr) { json.addArray("sampledValue"); for(size_t i = 0; i < sampledValue_length; ++i) { json.addObject(); sampledValue[i].serializeInto(json); json.endObject(); } json.endArray(); }
    }

void MeterValueSampledValue::serializeInto(TFJsonSerializer &json) {
        if (value != nullptr) json.add("value", value);
        if (context != SampledValueContext::NONE) json.add("context",SampledValueContextStrings[(size_t)context]);
        if (format != SampledValueFormat::NONE) json.add("format",SampledValueFormatStrings[(size_t)format]);
        if (measurand != SampledValueMeasurand::NONE) json.add("measurand",SampledValueMeasurandStrings[(size_t)measurand]);
        if (phase != SampledValuePhase::NONE) json.add("phase",SampledValuePhaseStrings[(size_t)phase]);
        if (location != SampledValueLocation::NONE) json.add("location",SampledValueLocationStrings[(size_t)location]);
        if (unit != SampledValueUnit::NONE) json.add("unit",SampledValueUnitStrings[(size_t)unit]);
    }



Authorize::Authorize(const char idTag[21]) :
    ICall(CallAction::AUTHORIZE, next_call_id++),
    idTag(idTag)
{}

size_t Authorize::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALL);
        json.add(this->ocppJmessageId, true);
        json.add(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (idTag != nullptr) json.add("idTag", idTag);
        json.endObject();
    json.endArray();

    return json.end();
}

BootNotification::BootNotification(const char chargePointVendor[21],
        const char chargePointModel[21],
        const char chargePointSerialNumber[26],
        const char chargeBoxSerialNumber[26],
        const char firmwareVersion[51],
        const char iccid[21],
        const char imsi[21],
        const char meterType[26],
        const char meterSerialNumber[26]) :
    ICall(CallAction::BOOT_NOTIFICATION, next_call_id++),
    chargePointVendor(chargePointVendor),
    chargePointModel(chargePointModel),
    chargePointSerialNumber(chargePointSerialNumber),
    chargeBoxSerialNumber(chargeBoxSerialNumber),
    firmwareVersion(firmwareVersion),
    iccid(iccid),
    imsi(imsi),
    meterType(meterType),
    meterSerialNumber(meterSerialNumber)
{}

size_t BootNotification::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALL);
        json.add(this->ocppJmessageId, true);
        json.add(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (chargePointVendor != nullptr) json.add("chargePointVendor", chargePointVendor);
            if (chargePointModel != nullptr) json.add("chargePointModel", chargePointModel);
            if (chargePointSerialNumber != nullptr) json.add("chargePointSerialNumber", chargePointSerialNumber);
            if (chargeBoxSerialNumber != nullptr) json.add("chargeBoxSerialNumber", chargeBoxSerialNumber);
            if (firmwareVersion != nullptr) json.add("firmwareVersion", firmwareVersion);
            if (iccid != nullptr) json.add("iccid", iccid);
            if (imsi != nullptr) json.add("imsi", imsi);
            if (meterType != nullptr) json.add("meterType", meterType);
            if (meterSerialNumber != nullptr) json.add("meterSerialNumber", meterSerialNumber);
        json.endObject();
    json.endArray();

    return json.end();
}

ChangeAvailabilityResponse::ChangeAvailabilityResponse(const char *call_id,
        ChangeAvailabilityResponseStatus status) :
    ICall(CallAction::CHANGE_AVAILABILITY_RESPONSE, call_id),
    status(status)
{}

size_t ChangeAvailabilityResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALLRESULT);
        json.add(this->ocppJcallId);

        json.addObject();
            if (status != ChangeAvailabilityResponseStatus::NONE) json.add("status",ChangeAvailabilityResponseStatusStrings[(size_t)status]);
        json.endObject();
    json.endArray();

    return json.end();
}

ChangeConfigurationResponse::ChangeConfigurationResponse(const char *call_id,
        ChangeConfigurationResponseStatus status) :
    ICall(CallAction::CHANGE_CONFIGURATION_RESPONSE, call_id),
    status(status)
{}

size_t ChangeConfigurationResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALLRESULT);
        json.add(this->ocppJcallId);

        json.addObject();
            if (status != ChangeConfigurationResponseStatus::NONE) json.add("status",ChangeConfigurationResponseStatusStrings[(size_t)status]);
        json.endObject();
    json.endArray();

    return json.end();
}

ClearCacheResponse::ClearCacheResponse(const char *call_id,
        ResponseStatus status) :
    ICall(CallAction::CLEAR_CACHE_RESPONSE, call_id),
    status(status)
{}

size_t ClearCacheResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALLRESULT);
        json.add(this->ocppJcallId);

        json.addObject();
            if (status != ResponseStatus::NONE) json.add("status",ResponseStatusStrings[(size_t)status]);
        json.endObject();
    json.endArray();

    return json.end();
}

DataTransfer::DataTransfer(const char vendorId[256],
        const char messageId[51],
        const char *data) :
    ICall(CallAction::DATA_TRANSFER, next_call_id++),
    vendorId(vendorId),
    messageId(messageId),
    data(data)
{}

size_t DataTransfer::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALL);
        json.add(this->ocppJmessageId, true);
        json.add(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (vendorId != nullptr) json.add("vendorId", vendorId);
            if (messageId != nullptr) json.add("messageId", messageId);
            if (data != nullptr) json.add("data", data);
        json.endObject();
    json.endArray();

    return json.end();
}

DataTransferResponse::DataTransferResponse(const char *call_id,
        DataTransferResponseStatus status,
        const char *data) :
    ICall(CallAction::DATA_TRANSFER_RESPONSE, call_id),
    status(status),
    data(data)
{}

size_t DataTransferResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALLRESULT);
        json.add(this->ocppJcallId);

        json.addObject();
            if (status != DataTransferResponseStatus::NONE) json.add("status",DataTransferResponseStatusStrings[(size_t)status]);
            if (data != nullptr) json.add("data", data);
        json.endObject();
    json.endArray();

    return json.end();
}

GetConfigurationResponse::GetConfigurationResponse(const char *call_id,
        GetConfigurationResponseConfigurationKey *configurationKey, size_t configurationKey_length,
        const char **unknownKey, size_t unknownKey_length) :
    ICall(CallAction::GET_CONFIGURATION_RESPONSE, call_id),
    configurationKey(configurationKey),
    configurationKey_length(configurationKey_length),
    unknownKey(unknownKey),
    unknownKey_length(unknownKey_length)
{}

size_t GetConfigurationResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALLRESULT);
        json.add(this->ocppJcallId);

        json.addObject();
            if (configurationKey != nullptr) { json.addArray("configurationKey"); for(size_t i = 0; i < configurationKey_length; ++i) { json.addObject(); configurationKey[i].serializeInto(json); json.endObject(); } json.endArray(); }
            if (unknownKey != nullptr) { json.addArray("unknownKey"); for(size_t i = 0; i < unknownKey_length; ++i) { json.add(unknownKey[i]); } json.endArray(); }
        json.endObject();
    json.endArray();

    return json.end();
}

Heartbeat::Heartbeat() :
    ICall(CallAction::HEARTBEAT, next_call_id++)
{}

size_t Heartbeat::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALL);
        json.add(this->ocppJmessageId, true);
        json.add(CallActionStrings[(size_t)this->action]);
        json.addObject();

        json.endObject();
    json.endArray();

    return json.end();
}

MeterValues::MeterValues(int32_t connectorId,
        MeterValue *meterValue, size_t meterValue_length,
        int32_t transactionId) :
    ICall(CallAction::METER_VALUES, next_call_id++),
    connectorId(connectorId),
    transactionId(transactionId),
    meterValue(meterValue),
    meterValue_length(meterValue_length)
{}

size_t MeterValues::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALL);
        json.add(this->ocppJmessageId, true);
        json.add(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (connectorId != OCPP_INTEGER_NOT_PASSED) json.add("connectorId", connectorId);
            if (transactionId != OCPP_INTEGER_NOT_PASSED) json.add("transactionId", transactionId);
            if (meterValue != nullptr) { json.addArray("meterValue"); for(size_t i = 0; i < meterValue_length; ++i) { json.addObject(); meterValue[i].serializeInto(json); json.endObject(); } json.endArray(); }
        json.endObject();
    json.endArray();

    return json.end();
}

RemoteStartTransactionResponse::RemoteStartTransactionResponse(const char *call_id,
        ResponseStatus status) :
    ICall(CallAction::REMOTE_START_TRANSACTION_RESPONSE, call_id),
    status(status)
{}

size_t RemoteStartTransactionResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALLRESULT);
        json.add(this->ocppJcallId);

        json.addObject();
            if (status != ResponseStatus::NONE) json.add("status",ResponseStatusStrings[(size_t)status]);
        json.endObject();
    json.endArray();

    return json.end();
}

RemoteStopTransactionResponse::RemoteStopTransactionResponse(const char *call_id,
        ResponseStatus status) :
    ICall(CallAction::REMOTE_STOP_TRANSACTION_RESPONSE, call_id),
    status(status)
{}

size_t RemoteStopTransactionResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALLRESULT);
        json.add(this->ocppJcallId);

        json.addObject();
            if (status != ResponseStatus::NONE) json.add("status",ResponseStatusStrings[(size_t)status]);
        json.endObject();
    json.endArray();

    return json.end();
}

ResetResponse::ResetResponse(const char *call_id,
        ResponseStatus status) :
    ICall(CallAction::RESET_RESPONSE, call_id),
    status(status)
{}

size_t ResetResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALLRESULT);
        json.add(this->ocppJcallId);

        json.addObject();
            if (status != ResponseStatus::NONE) json.add("status",ResponseStatusStrings[(size_t)status]);
        json.endObject();
    json.endArray();

    return json.end();
}

StartTransaction::StartTransaction(int32_t connectorId,
        const char idTag[21],
        int32_t meterStart,
        time_t timestamp,
        int32_t reservationId) :
    ICall(CallAction::START_TRANSACTION, next_call_id++),
    connectorId(connectorId),
    idTag(idTag),
    meterStart(meterStart),
    reservationId(reservationId),
    timestamp(timestamp)
{}

size_t StartTransaction::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALL);
        json.add(this->ocppJmessageId, true);
        json.add(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (connectorId != OCPP_INTEGER_NOT_PASSED) json.add("connectorId", connectorId);
            if (idTag != nullptr) json.add("idTag", idTag);
            if (meterStart != OCPP_INTEGER_NOT_PASSED) json.add("meterStart", meterStart);
            if (reservationId != OCPP_INTEGER_NOT_PASSED) json.add("reservationId", reservationId);
            if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, json, "timestamp");
        json.endObject();
    json.endArray();

    return json.end();
}

StatusNotification::StatusNotification(int32_t connectorId,
        StatusNotificationErrorCode errorCode,
        StatusNotificationStatus status,
        const char info[51],
        time_t timestamp,
        const char vendorId[256],
        const char vendorErrorCode[51]) :
    ICall(CallAction::STATUS_NOTIFICATION, next_call_id++),
    connectorId(connectorId),
    errorCode(errorCode),
    info(info),
    status(status),
    timestamp(timestamp),
    vendorId(vendorId),
    vendorErrorCode(vendorErrorCode)
{}

size_t StatusNotification::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALL);
        json.add(this->ocppJmessageId, true);
        json.add(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (connectorId != OCPP_INTEGER_NOT_PASSED) json.add("connectorId", connectorId);
            if (errorCode != StatusNotificationErrorCode::NONE) json.add("errorCode",StatusNotificationErrorCodeStrings[(size_t)errorCode]);
            if (info != nullptr) json.add("info", info);
            if (status != StatusNotificationStatus::NONE) json.add("status",StatusNotificationStatusStrings[(size_t)status]);
            if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, json, "timestamp");
            if (vendorId != nullptr) json.add("vendorId", vendorId);
            if (vendorErrorCode != nullptr) json.add("vendorErrorCode", vendorErrorCode);
        json.endObject();
    json.endArray();

    return json.end();
}

StopTransaction::StopTransaction(int32_t meterStop,
        time_t timestamp,
        int32_t transactionId,
        const char idTag[21],
        StopTransactionReason reason,
        MeterValue *transactionData, size_t transactionData_length) :
    ICall(CallAction::STOP_TRANSACTION, next_call_id++),
    idTag(idTag),
    meterStop(meterStop),
    timestamp(timestamp),
    transactionId(transactionId),
    reason(reason),
    transactionData(transactionData),
    transactionData_length(transactionData_length)
{}

size_t StopTransaction::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALL);
        json.add(this->ocppJmessageId, true);
        json.add(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (idTag != nullptr) json.add("idTag", idTag);
            if (meterStop != OCPP_INTEGER_NOT_PASSED) json.add("meterStop", meterStop);
            if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, json, "timestamp");
            if (transactionId != OCPP_INTEGER_NOT_PASSED) json.add("transactionId", transactionId);
            if (reason != StopTransactionReason::NONE) json.add("reason",StopTransactionReasonStrings[(size_t)reason]);
            if (transactionData != nullptr) { json.addArray("transactionData"); for(size_t i = 0; i < transactionData_length; ++i) { json.addObject(); transactionData[i].serializeInto(json); json.endObject(); } json.endArray(); }
        json.endObject();
    json.endArray();

    return json.end();
}

UnlockConnectorResponse::UnlockConnectorResponse(const char *call_id,
        UnlockConnectorResponseStatus status) :
    ICall(CallAction::UNLOCK_CONNECTOR_RESPONSE, call_id),
    status(status)
{}

size_t UnlockConnectorResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.add((int32_t)OcppRpcMessageType::CALLRESULT);
        json.add(this->ocppJcallId);

        json.addObject();
            if (status != UnlockConnectorResponseStatus::NONE) json.add("status",UnlockConnectorResponseStatusStrings[(size_t)status]);
        json.endObject();
    json.endArray();

    return json.end();
}

static CallResponse parseAuthorizeResponseIdTagInfoEntriesExpiryDate(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "expiryDate: wrong type"};

    {
        time_t result;
        if (!iso_string_to_unix_timestamp(var.as<const char *>(), &result))
            return CallResponse{CallErrorCode::TypeConstraintViolation, "expiryDate: failed to parse as ISO 8601 date-time string"};

        var.set(result);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseAuthorizeResponseIdTagInfoEntriesParentIdTag(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "parentIdTag: wrong type"};

    if (strlen(var.as<const char *>()) > 20)
        return CallResponse{CallErrorCode::PropertyConstraintViolation, "parentIdTag: string too long"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseAuthorizeResponseIdTagInfoEntriesStatus(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "status: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(ResponseIdTagInfoEntriesStatusStrings); ++i) {
            if (strcmp(var.as<const char *>(), ResponseIdTagInfoEntriesStatusStrings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }

        if (!found)
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "status: unknown enum value received"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseAuthorizeResponseIdTagInfoEntries(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.containsKey("expiryDate")) {
    {
        CallResponse inner_result = parseAuthorizeResponseIdTagInfoEntriesExpiryDate(obj["expiryDate"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("parentIdTag")) {
    {
        CallResponse inner_result = parseAuthorizeResponseIdTagInfoEntriesParentIdTag(obj["parentIdTag"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (!obj.containsKey("status"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "status: required, but missing"};

    {
        CallResponse inner_result = parseAuthorizeResponseIdTagInfoEntriesStatus(obj["status"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "AuthorizeResponseIdTagInfoEntries: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseAuthorizeResponseIdTagInfo(JsonVariant var) {

    if (!var.is<JsonObject>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "idTagInfo: wrong type"};

    {
        CallResponse inner_result = parseAuthorizeResponseIdTagInfoEntries(var);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseAuthorizeResponse(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("idTagInfo"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "idTagInfo: required, but missing"};

    {
        CallResponse inner_result = parseAuthorizeResponseIdTagInfo(obj["idTagInfo"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "AuthorizeResponse: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseBootNotificationResponseStatus(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "status: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(BootNotificationResponseStatusStrings); ++i) {
            if (strcmp(var.as<const char *>(), BootNotificationResponseStatusStrings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }

        if (!found)
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "status: unknown enum value received"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseBootNotificationResponseCurrentTime(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "currentTime: wrong type"};

    {
        time_t result;
        if (!iso_string_to_unix_timestamp(var.as<const char *>(), &result))
            return CallResponse{CallErrorCode::TypeConstraintViolation, "currentTime: failed to parse as ISO 8601 date-time string"};

        var.set(result);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseBootNotificationResponseInterval(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "interval: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseBootNotificationResponse(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("status"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "status: required, but missing"};

    {
        CallResponse inner_result = parseBootNotificationResponseStatus(obj["status"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("currentTime"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "currentTime: required, but missing"};

    {
        CallResponse inner_result = parseBootNotificationResponseCurrentTime(obj["currentTime"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("interval"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "interval: required, but missing"};

    {
        CallResponse inner_result = parseBootNotificationResponseInterval(obj["interval"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "BootNotificationResponse: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseChangeAvailabilityConnectorId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "connectorId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseChangeAvailabilityType(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "type: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(ChangeAvailabilityTypeStrings); ++i) {
            if (strcmp(var.as<const char *>(), ChangeAvailabilityTypeStrings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }

        if (!found)
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "type: unknown enum value received"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseChangeAvailability(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("connectorId"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "connectorId: required, but missing"};

    {
        CallResponse inner_result = parseChangeAvailabilityConnectorId(obj["connectorId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("type"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "type: required, but missing"};

    {
        CallResponse inner_result = parseChangeAvailabilityType(obj["type"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "ChangeAvailability: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseChangeConfigurationKey(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "key: wrong type"};

    if (strlen(var.as<const char *>()) > 50)
        return CallResponse{CallErrorCode::PropertyConstraintViolation, "key: string too long"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseChangeConfigurationValue(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "value: wrong type"};

    if (strlen(var.as<const char *>()) > 500)
        return CallResponse{CallErrorCode::PropertyConstraintViolation, "value: string too long"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseChangeConfiguration(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("key"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "key: required, but missing"};

    {
        CallResponse inner_result = parseChangeConfigurationKey(obj["key"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("value"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "value: required, but missing"};

    {
        CallResponse inner_result = parseChangeConfigurationValue(obj["value"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "ChangeConfiguration: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse parseClearCache(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "ClearCache: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseDataTransferVendorId(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "vendorId: wrong type"};

    if (strlen(var.as<const char *>()) > 255)
        return CallResponse{CallErrorCode::PropertyConstraintViolation, "vendorId: string too long"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseDataTransferMessageId(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "messageId: wrong type"};

    if (strlen(var.as<const char *>()) > 50)
        return CallResponse{CallErrorCode::PropertyConstraintViolation, "messageId: string too long"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseDataTransferData(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "data: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseDataTransfer(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("vendorId"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "vendorId: required, but missing"};

    {
        CallResponse inner_result = parseDataTransferVendorId(obj["vendorId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("messageId")) {
    {
        CallResponse inner_result = parseDataTransferMessageId(obj["messageId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("data")) {
    {
        CallResponse inner_result = parseDataTransferData(obj["data"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "DataTransfer: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseDataTransferResponseStatus(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "status: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(DataTransferResponseStatusStrings); ++i) {
            if (strcmp(var.as<const char *>(), DataTransferResponseStatusStrings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }

        if (!found)
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "status: unknown enum value received"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseDataTransferResponseData(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "data: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseDataTransferResponse(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("status"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "status: required, but missing"};

    {
        CallResponse inner_result = parseDataTransferResponseStatus(obj["status"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("data")) {
    {
        CallResponse inner_result = parseDataTransferResponseData(obj["data"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "DataTransferResponse: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseGetConfigurationKeyEntry(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "keyEntry: wrong type"};

    if (strlen(var.as<const char *>()) > 50)
        return CallResponse{CallErrorCode::PropertyConstraintViolation, "keyEntry: string too long"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseGetConfigurationKey(JsonVariant var) {

    if (!var.is<JsonArray>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "key: wrong type"};

    {
        for(size_t i = 0; i < var.as<JsonArray>().size(); ++i) {
            CallResponse inner_result = parseGetConfigurationKeyEntry(var[i]);
            if (inner_result.result != CallErrorCode::OK)
                return inner_result;
        }
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseGetConfiguration(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.containsKey("key")) {
    {
        CallResponse inner_result = parseGetConfigurationKey(obj["key"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "GetConfiguration: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseHeartbeatResponseCurrentTime(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "currentTime: wrong type"};

    {
        time_t result;
        if (!iso_string_to_unix_timestamp(var.as<const char *>(), &result))
            return CallResponse{CallErrorCode::TypeConstraintViolation, "currentTime: failed to parse as ISO 8601 date-time string"};

        var.set(result);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseHeartbeatResponse(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("currentTime"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "currentTime: required, but missing"};

    {
        CallResponse inner_result = parseHeartbeatResponseCurrentTime(obj["currentTime"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "HeartbeatResponse: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse parseMeterValuesResponse(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "MeterValuesResponse: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionConnectorId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "connectorId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionIdTag(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "idTag: wrong type"};

    if (strlen(var.as<const char *>()) > 20)
        return CallResponse{CallErrorCode::PropertyConstraintViolation, "idTag: string too long"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingProfileId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingProfileId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesTransactionId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "transactionId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesStackLevel(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "stackLevel: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingProfilePurpose(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingProfilePurpose: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(RemoteStartTransactionChargingProfileEntriesChargingProfilePurposeStrings); ++i) {
            if (strcmp(var.as<const char *>(), RemoteStartTransactionChargingProfileEntriesChargingProfilePurposeStrings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }

        if (!found)
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "chargingProfilePurpose: unknown enum value received"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingProfileKind(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingProfileKind: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(RemoteStartTransactionChargingProfileEntriesChargingProfileKindStrings); ++i) {
            if (strcmp(var.as<const char *>(), RemoteStartTransactionChargingProfileEntriesChargingProfileKindStrings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }

        if (!found)
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "chargingProfileKind: unknown enum value received"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesRecurrencyKind(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "recurrencyKind: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(RemoteStartTransactionChargingProfileEntriesRecurrencyKindStrings); ++i) {
            if (strcmp(var.as<const char *>(), RemoteStartTransactionChargingProfileEntriesRecurrencyKindStrings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }

        if (!found)
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "recurrencyKind: unknown enum value received"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesValidFrom(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "validFrom: wrong type"};

    {
        time_t result;
        if (!iso_string_to_unix_timestamp(var.as<const char *>(), &result))
            return CallResponse{CallErrorCode::TypeConstraintViolation, "validFrom: failed to parse as ISO 8601 date-time string"};

        var.set(result);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesValidTo(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "validTo: wrong type"};

    {
        time_t result;
        if (!iso_string_to_unix_timestamp(var.as<const char *>(), &result))
            return CallResponse{CallErrorCode::TypeConstraintViolation, "validTo: failed to parse as ISO 8601 date-time string"};

        var.set(result);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesDuration(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "duration: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesStartSchedule(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "startSchedule: wrong type"};

    {
        time_t result;
        if (!iso_string_to_unix_timestamp(var.as<const char *>(), &result))
            return CallResponse{CallErrorCode::TypeConstraintViolation, "startSchedule: failed to parse as ISO 8601 date-time string"};

        var.set(result);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingRateUnit(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingRateUnit: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingRateUnitStrings); ++i) {
            if (strcmp(var.as<const char *>(), RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingRateUnitStrings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }

        if (!found)
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "chargingRateUnit: unknown enum value received"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesStartPeriod(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "startPeriod: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesLimit(JsonVariant var) {

    if (!var.is<float>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "limit: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesNumberPhases(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "numberPhases: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntries(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("startPeriod"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "startPeriod: required, but missing"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesStartPeriod(obj["startPeriod"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("limit"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "limit: required, but missing"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesLimit(obj["limit"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("numberPhases")) {
    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesNumberPhases(obj["numberPhases"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntries: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntry(JsonVariant var) {

    if (!var.is<JsonObject>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingSchedulePeriodEntry: wrong type"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntries(var);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriod(JsonVariant var) {

    if (!var.is<JsonArray>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingSchedulePeriod: wrong type"};

    {
        for(size_t i = 0; i < var.as<JsonArray>().size(); ++i) {
            CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntry(var[i]);
            if (inner_result.result != CallErrorCode::OK)
                return inner_result;
        }
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesMinChargingRate(JsonVariant var) {

    if (!var.is<float>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "minChargingRate: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntries(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.containsKey("duration")) {
    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesDuration(obj["duration"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("startSchedule")) {
    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesStartSchedule(obj["startSchedule"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (!obj.containsKey("chargingRateUnit"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingRateUnit: required, but missing"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingRateUnit(obj["chargingRateUnit"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("chargingSchedulePeriod"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingSchedulePeriod: required, but missing"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriod(obj["chargingSchedulePeriod"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("minChargingRate")) {
    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesMinChargingRate(obj["minChargingRate"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "RemoteStartTransactionChargingProfileEntriesChargingScheduleEntries: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseRemoteStartTransactionChargingProfileEntriesChargingSchedule(JsonVariant var) {

    if (!var.is<JsonObject>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingSchedule: wrong type"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingScheduleEntries(var);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseRemoteStartTransactionChargingProfileEntries(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("chargingProfileId"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingProfileId: required, but missing"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingProfileId(obj["chargingProfileId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("transactionId")) {
    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesTransactionId(obj["transactionId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (!obj.containsKey("stackLevel"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "stackLevel: required, but missing"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesStackLevel(obj["stackLevel"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("chargingProfilePurpose"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingProfilePurpose: required, but missing"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingProfilePurpose(obj["chargingProfilePurpose"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("chargingProfileKind"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingProfileKind: required, but missing"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingProfileKind(obj["chargingProfileKind"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("recurrencyKind")) {
    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesRecurrencyKind(obj["recurrencyKind"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("validFrom")) {
    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesValidFrom(obj["validFrom"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("validTo")) {
    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesValidTo(obj["validTo"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (!obj.containsKey("chargingSchedule"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingSchedule: required, but missing"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntriesChargingSchedule(obj["chargingSchedule"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "RemoteStartTransactionChargingProfileEntries: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseRemoteStartTransactionChargingProfile(JsonVariant var) {

    if (!var.is<JsonObject>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingProfile: wrong type"};

    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileEntries(var);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseRemoteStartTransaction(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.containsKey("connectorId")) {
    {
        CallResponse inner_result = parseRemoteStartTransactionConnectorId(obj["connectorId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (!obj.containsKey("idTag"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "idTag: required, but missing"};

    {
        CallResponse inner_result = parseRemoteStartTransactionIdTag(obj["idTag"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("chargingProfile")) {
    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfile(obj["chargingProfile"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "RemoteStartTransaction: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseRemoteStopTransactionTransactionId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "transactionId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseRemoteStopTransaction(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("transactionId"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "transactionId: required, but missing"};

    {
        CallResponse inner_result = parseRemoteStopTransactionTransactionId(obj["transactionId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "RemoteStopTransaction: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseResetType(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "type: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(ResetTypeStrings); ++i) {
            if (strcmp(var.as<const char *>(), ResetTypeStrings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }

        if (!found)
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "type: unknown enum value received"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseReset(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("type"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "type: required, but missing"};

    {
        CallResponse inner_result = parseResetType(obj["type"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "Reset: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseStartTransactionResponseIdTagInfoEntriesExpiryDate(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "expiryDate: wrong type"};

    {
        time_t result;
        if (!iso_string_to_unix_timestamp(var.as<const char *>(), &result))
            return CallResponse{CallErrorCode::TypeConstraintViolation, "expiryDate: failed to parse as ISO 8601 date-time string"};

        var.set(result);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseStartTransactionResponseIdTagInfoEntriesParentIdTag(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "parentIdTag: wrong type"};

    if (strlen(var.as<const char *>()) > 20)
        return CallResponse{CallErrorCode::PropertyConstraintViolation, "parentIdTag: string too long"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseStartTransactionResponseIdTagInfoEntriesStatus(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "status: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(ResponseIdTagInfoEntriesStatusStrings); ++i) {
            if (strcmp(var.as<const char *>(), ResponseIdTagInfoEntriesStatusStrings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }

        if (!found)
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "status: unknown enum value received"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseStartTransactionResponseIdTagInfoEntries(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.containsKey("expiryDate")) {
    {
        CallResponse inner_result = parseStartTransactionResponseIdTagInfoEntriesExpiryDate(obj["expiryDate"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("parentIdTag")) {
    {
        CallResponse inner_result = parseStartTransactionResponseIdTagInfoEntriesParentIdTag(obj["parentIdTag"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (!obj.containsKey("status"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "status: required, but missing"};

    {
        CallResponse inner_result = parseStartTransactionResponseIdTagInfoEntriesStatus(obj["status"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "StartTransactionResponseIdTagInfoEntries: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseStartTransactionResponseIdTagInfo(JsonVariant var) {

    if (!var.is<JsonObject>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "idTagInfo: wrong type"};

    {
        CallResponse inner_result = parseStartTransactionResponseIdTagInfoEntries(var);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseStartTransactionResponseTransactionId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "transactionId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseStartTransactionResponse(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("idTagInfo"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "idTagInfo: required, but missing"};

    {
        CallResponse inner_result = parseStartTransactionResponseIdTagInfo(obj["idTagInfo"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("transactionId"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "transactionId: required, but missing"};

    {
        CallResponse inner_result = parseStartTransactionResponseTransactionId(obj["transactionId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "StartTransactionResponse: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse parseStatusNotificationResponse(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "StatusNotificationResponse: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseStopTransactionResponseIdTagInfoEntriesExpiryDate(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "expiryDate: wrong type"};

    {
        time_t result;
        if (!iso_string_to_unix_timestamp(var.as<const char *>(), &result))
            return CallResponse{CallErrorCode::TypeConstraintViolation, "expiryDate: failed to parse as ISO 8601 date-time string"};

        var.set(result);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseStopTransactionResponseIdTagInfoEntriesParentIdTag(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "parentIdTag: wrong type"};

    if (strlen(var.as<const char *>()) > 20)
        return CallResponse{CallErrorCode::PropertyConstraintViolation, "parentIdTag: string too long"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseStopTransactionResponseIdTagInfoEntriesStatus(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "status: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(ResponseIdTagInfoEntriesStatusStrings); ++i) {
            if (strcmp(var.as<const char *>(), ResponseIdTagInfoEntriesStatusStrings[i]) != 0)
                continue;

            var.set(i);
            found = true;
            break;
        }

        if (!found)
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "status: unknown enum value received"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseStopTransactionResponseIdTagInfoEntries(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.containsKey("expiryDate")) {
    {
        CallResponse inner_result = parseStopTransactionResponseIdTagInfoEntriesExpiryDate(obj["expiryDate"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("parentIdTag")) {
    {
        CallResponse inner_result = parseStopTransactionResponseIdTagInfoEntriesParentIdTag(obj["parentIdTag"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (!obj.containsKey("status"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "status: required, but missing"};

    {
        CallResponse inner_result = parseStopTransactionResponseIdTagInfoEntriesStatus(obj["status"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "StopTransactionResponseIdTagInfoEntries: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseStopTransactionResponseIdTagInfo(JsonVariant var) {

    if (!var.is<JsonObject>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "idTagInfo: wrong type"};

    {
        CallResponse inner_result = parseStopTransactionResponseIdTagInfoEntries(var);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseStopTransactionResponse(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.containsKey("idTagInfo")) {
    {
        CallResponse inner_result = parseStopTransactionResponseIdTagInfo(obj["idTagInfo"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "StopTransactionResponse: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseUnlockConnectorConnectorId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "connectorId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseUnlockConnector(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("connectorId"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "connectorId: required, but missing"};

    {
        CallResponse inner_result = parseUnlockConnectorConnectorId(obj["connectorId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "UnlockConnector: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse callHandler(const char *uid, const char *action_string, JsonObject obj, OcppChargePoint *cp) {
    size_t action_idx = 0;
    if (!lookup_key(&action_idx, action_string, CallActionStrings, ARRAY_SIZE(CallActionStrings)))
        return CallResponse{CallErrorCode::NotImplemented, "unknown action passed"};

    CallAction action = (CallAction) action_idx;

    switch(action) {
        case CallAction::CHANGE_AVAILABILITY: {
            CallResponse res = parseChangeAvailability(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleChangeAvailability(uid, ChangeAvailabilityView{obj});
        }

        case CallAction::CHANGE_CONFIGURATION: {
            CallResponse res = parseChangeConfiguration(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleChangeConfiguration(uid, ChangeConfigurationView{obj});
        }

        case CallAction::CLEAR_CACHE: {
            CallResponse res = parseClearCache(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleClearCache(uid, ClearCacheView{obj});
        }

        case CallAction::DATA_TRANSFER: {
            CallResponse res = parseDataTransfer(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleDataTransfer(uid, DataTransferView{obj});
        }

        case CallAction::GET_CONFIGURATION: {
            CallResponse res = parseGetConfiguration(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleGetConfiguration(uid, GetConfigurationView{obj});
        }

        case CallAction::REMOTE_START_TRANSACTION: {
            CallResponse res = parseRemoteStartTransaction(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleRemoteStartTransaction(uid, RemoteStartTransactionView{obj});
        }

        case CallAction::REMOTE_STOP_TRANSACTION: {
            CallResponse res = parseRemoteStopTransaction(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleRemoteStopTransaction(uid, RemoteStopTransactionView{obj});
        }

        case CallAction::RESET: {
            CallResponse res = parseReset(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleReset(uid, ResetView{obj});
        }

        case CallAction::UNLOCK_CONNECTOR: {
            CallResponse res = parseUnlockConnector(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleUnlockConnector(uid, UnlockConnectorView{obj});
        }

        case CallAction::AUTHORIZE:
        case CallAction::BOOT_NOTIFICATION:
        case CallAction::CHANGE_AVAILABILITY_RESPONSE:
        case CallAction::CHANGE_CONFIGURATION_RESPONSE:
        case CallAction::CLEAR_CACHE_RESPONSE:
        case CallAction::GET_CONFIGURATION_RESPONSE:
        case CallAction::HEARTBEAT:
        case CallAction::METER_VALUES:
        case CallAction::REMOTE_START_TRANSACTION_RESPONSE:
        case CallAction::REMOTE_STOP_TRANSACTION_RESPONSE:
        case CallAction::RESET_RESPONSE:
        case CallAction::START_TRANSACTION:
        case CallAction::STATUS_NOTIFICATION:
        case CallAction::STOP_TRANSACTION:
        case CallAction::UNLOCK_CONNECTOR_RESPONSE:
        case CallAction::AUTHORIZE_RESPONSE:
        case CallAction::BOOT_NOTIFICATION_RESPONSE:
        case CallAction::DATA_TRANSFER_RESPONSE:
        case CallAction::HEARTBEAT_RESPONSE:
        case CallAction::METER_VALUES_RESPONSE:
        case CallAction::START_TRANSACTION_RESPONSE:
        case CallAction::STATUS_NOTIFICATION_RESPONSE:
        case CallAction::STOP_TRANSACTION_RESPONSE:
        case CallAction::GET_DIAGNOSTICS_RESPONSE:
        case CallAction::DIAGNOSTICS_STATUS_NOTIFICATION:
        case CallAction::FIRMWARE_STATUS_NOTIFICATION:
        case CallAction::UPDATE_FIRMWARE_RESPONSE:
        case CallAction::GET_DIAGNOSTICS:
        case CallAction::DIAGNOSTICS_STATUS_NOTIFICATION_RESPONSE:
        case CallAction::FIRMWARE_STATUS_NOTIFICATION_RESPONSE:
        case CallAction::UPDATE_FIRMWARE:
        case CallAction::GET_LOCAL_LIST_VERSION_RESPONSE:
        case CallAction::SEND_LOCAL_LIST_RESPONSE:
        case CallAction::GET_LOCAL_LIST_VERSION:
        case CallAction::SEND_LOCAL_LIST:
        case CallAction::CANCEL_RESERVATION_RESPONSE:
        case CallAction::RESERVE_NOW_RESPONSE:
        case CallAction::CANCEL_RESERVATION:
        case CallAction::RESERVE_NOW:
        case CallAction::CLEAR_CHARGING_PROFILE_RESPONSE:
        case CallAction::GET_COMPOSITE_SCHEDULE_RESPONSE:
        case CallAction::SET_CHARGING_PROFILE_RESPONSE:
        case CallAction::CLEAR_CHARGING_PROFILE:
        case CallAction::GET_COMPOSITE_SCHEDULE:
        case CallAction::SET_CHARGING_PROFILE:
        case CallAction::TRIGGER_MESSAGE_RESPONSE:
        case CallAction::TRIGGER_MESSAGE:
            return CallResponse{CallErrorCode::NotSupported, "action not supported"};
    }
}

CallResponse callResultHandler(int32_t connectorId, CallAction resultTo, JsonObject obj, OcppChargePoint *cp) {

    switch(resultTo) {
        case CallAction::AUTHORIZE: {
            CallResponse res = parseAuthorizeResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleAuthorizeResponse(connectorId, AuthorizeResponseView{obj});
        }

        case CallAction::BOOT_NOTIFICATION: {
            CallResponse res = parseBootNotificationResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleBootNotificationResponse(connectorId, BootNotificationResponseView{obj});
        }

        case CallAction::DATA_TRANSFER: {
            CallResponse res = parseDataTransferResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleDataTransferResponse(connectorId, DataTransferResponseView{obj});
        }

        case CallAction::HEARTBEAT: {
            CallResponse res = parseHeartbeatResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleHeartbeatResponse(connectorId, HeartbeatResponseView{obj});
        }

        case CallAction::METER_VALUES: {
            CallResponse res = parseMeterValuesResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleMeterValuesResponse(connectorId, MeterValuesResponseView{obj});
        }

        case CallAction::START_TRANSACTION: {
            CallResponse res = parseStartTransactionResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleStartTransactionResponse(connectorId, StartTransactionResponseView{obj});
        }

        case CallAction::STATUS_NOTIFICATION: {
            CallResponse res = parseStatusNotificationResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleStatusNotificationResponse(connectorId, StatusNotificationResponseView{obj});
        }

        case CallAction::STOP_TRANSACTION: {
            CallResponse res = parseStopTransactionResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleStopTransactionResponse(connectorId, StopTransactionResponseView{obj});
        }

        case CallAction::CHANGE_AVAILABILITY_RESPONSE:
        case CallAction::CHANGE_CONFIGURATION_RESPONSE:
        case CallAction::CLEAR_CACHE_RESPONSE:
        case CallAction::GET_CONFIGURATION_RESPONSE:
        case CallAction::REMOTE_START_TRANSACTION_RESPONSE:
        case CallAction::REMOTE_STOP_TRANSACTION_RESPONSE:
        case CallAction::RESET_RESPONSE:
        case CallAction::UNLOCK_CONNECTOR_RESPONSE:
        case CallAction::AUTHORIZE_RESPONSE:
        case CallAction::BOOT_NOTIFICATION_RESPONSE:
        case CallAction::CHANGE_AVAILABILITY:
        case CallAction::CHANGE_CONFIGURATION:
        case CallAction::CLEAR_CACHE:
        case CallAction::DATA_TRANSFER_RESPONSE:
        case CallAction::GET_CONFIGURATION:
        case CallAction::HEARTBEAT_RESPONSE:
        case CallAction::METER_VALUES_RESPONSE:
        case CallAction::REMOTE_START_TRANSACTION:
        case CallAction::REMOTE_STOP_TRANSACTION:
        case CallAction::RESET:
        case CallAction::START_TRANSACTION_RESPONSE:
        case CallAction::STATUS_NOTIFICATION_RESPONSE:
        case CallAction::STOP_TRANSACTION_RESPONSE:
        case CallAction::UNLOCK_CONNECTOR:
        case CallAction::GET_DIAGNOSTICS_RESPONSE:
        case CallAction::DIAGNOSTICS_STATUS_NOTIFICATION:
        case CallAction::FIRMWARE_STATUS_NOTIFICATION:
        case CallAction::UPDATE_FIRMWARE_RESPONSE:
        case CallAction::GET_DIAGNOSTICS:
        case CallAction::DIAGNOSTICS_STATUS_NOTIFICATION_RESPONSE:
        case CallAction::FIRMWARE_STATUS_NOTIFICATION_RESPONSE:
        case CallAction::UPDATE_FIRMWARE:
        case CallAction::GET_LOCAL_LIST_VERSION_RESPONSE:
        case CallAction::SEND_LOCAL_LIST_RESPONSE:
        case CallAction::GET_LOCAL_LIST_VERSION:
        case CallAction::SEND_LOCAL_LIST:
        case CallAction::CANCEL_RESERVATION_RESPONSE:
        case CallAction::RESERVE_NOW_RESPONSE:
        case CallAction::CANCEL_RESERVATION:
        case CallAction::RESERVE_NOW:
        case CallAction::CLEAR_CHARGING_PROFILE_RESPONSE:
        case CallAction::GET_COMPOSITE_SCHEDULE_RESPONSE:
        case CallAction::SET_CHARGING_PROFILE_RESPONSE:
        case CallAction::CLEAR_CHARGING_PROFILE:
        case CallAction::GET_COMPOSITE_SCHEDULE:
        case CallAction::SET_CHARGING_PROFILE:
        case CallAction::TRIGGER_MESSAGE_RESPONSE:
        case CallAction::TRIGGER_MESSAGE:
            return CallResponse{CallErrorCode::NotSupported, "action not supported"};
    }
}
