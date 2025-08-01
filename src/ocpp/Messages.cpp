// THIS FILE IS AUTO-GENERATED. DO NOT EDIT.

#include "Messages.h"

#include "ChargePoint.h"
#include "Platform.h"
#include "Types.h"
#include "Tools.h"

extern "C" {
#include "lib/libiso8601/iso8601.h"
}

#include "TFJson.h"

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
    tm t;
    gmtime_r(&timestamp, &t);

    strftime(buf, ARRAY_SIZE(buf), "%FT%TZ", &t);

    json.addMemberString(key, buf);
}

ICall::~ICall() {}

size_t ICall::measureJson() const {
    return this->serializeJson(nullptr, 0);
}

uint64_t next_call_id = 0;

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

const char * const ChargingProfilePurposeStrings[] = {
    "ChargePointMaxProfile",
    "TxDefaultProfile",
    "TxProfile"
};

const char * const ChargingProfileKindStrings[] = {
    "Absolute",
    "Recurring",
    "Relative"
};

const char * const RecurrencyKindStrings[] = {
    "Daily",
    "Weekly"
};

const char * const ChargingRateUnitStrings[] = {
    "A",
    "W"
};

const char * const ResetTypeStrings[] = {
    "Hard",
    "Soft"
};

const char * const ClearChargingProfileResponseStatusStrings[] = {
    "Accepted",
    "Unknown"
};

const char * const SetChargingProfileResponseStatusStrings[] = {
    "Accepted",
    "Rejected",
    "NotSupported"
};

const char * const GetCompositeScheduleResponseChargingScheduleChargingRateUnitStrings[] = {
    "A",
    "W"
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
    "Celcius",
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
        if (key != nullptr) json.addMemberString("key", key);
        json.addMemberBoolean("readonly", readonly);
        if (value != nullptr) json.addMemberString("value", value);
    }

void MeterValue::serializeInto(TFJsonSerializer &json) {
        if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, json, "timestamp");
        if (sampledValue != nullptr) { json.addMemberArray("sampledValue"); for(size_t i = 0; i < sampledValue_length; ++i) { json.addObject(); sampledValue[i].serializeInto(json); json.endObject(); } json.endArray(); }
    }

void GetCompositeScheduleResponseChargingSchedule::serializeInto(TFJsonSerializer &json) {
        if (duration != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("duration", duration);
        if (startSchedule != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(startSchedule, json, "startSchedule");
        if (chargingRateUnit != GetCompositeScheduleResponseChargingScheduleChargingRateUnit::NONE) json.addMemberString("chargingRateUnit", GetCompositeScheduleResponseChargingScheduleChargingRateUnitStrings[(size_t)chargingRateUnit]);
        if (chargingSchedulePeriod != nullptr) { json.addMemberArray("chargingSchedulePeriod"); for(size_t i = 0; i < chargingSchedulePeriod_length; ++i) { json.addObject(); chargingSchedulePeriod[i].serializeInto(json); json.endObject(); } json.endArray(); }
        if (!isnan(minChargingRate)) json.addMemberNumber("minChargingRate", minChargingRate, "%.1f");
    }

void MeterValueSampledValue::serializeInto(TFJsonSerializer &json) {
        if (value != nullptr) json.addMemberString("value", value);
        if (context != SampledValueContext::NONE) json.addMemberString("context", SampledValueContextStrings[(size_t)context]);
        if (format != SampledValueFormat::NONE) json.addMemberString("format", SampledValueFormatStrings[(size_t)format]);
        if (measurand != SampledValueMeasurand::NONE) json.addMemberString("measurand", SampledValueMeasurandStrings[(size_t)measurand]);
        if (phase != SampledValuePhase::NONE) json.addMemberString("phase", SampledValuePhaseStrings[(size_t)phase]);
        if (location != SampledValueLocation::NONE) json.addMemberString("location", SampledValueLocationStrings[(size_t)location]);
        if (unit != SampledValueUnit::NONE) json.addMemberString("unit", SampledValueUnitStrings[(size_t)unit]);
    }

void GetCompositeScheduleResponseChargingScheduleChargingSchedulePeriod::serializeInto(TFJsonSerializer &json) {
        if (startPeriod != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("startPeriod", startPeriod);
        if (!isnan(limit)) json.addMemberNumber("limit", limit, "%.1f");
        if (numberPhases != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("numberPhases", numberPhases);
    }



Authorize::Authorize(const char idTag[21]) :
    ICall(CallAction::AUTHORIZE, next_call_id++),
    idTag(idTag)
{}

size_t Authorize::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.addNumber((int32_t)OcppRpcMessageType::CALL);
        json.addNumber(this->ocppJmessageId, true);
        json.addString(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (idTag != nullptr) json.addMemberString("idTag", idTag);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALL);
        json.addNumber(this->ocppJmessageId, true);
        json.addString(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (chargePointVendor != nullptr) json.addMemberString("chargePointVendor", chargePointVendor);
            if (chargePointModel != nullptr) json.addMemberString("chargePointModel", chargePointModel);
            if (chargePointSerialNumber != nullptr) json.addMemberString("chargePointSerialNumber", chargePointSerialNumber);
            if (chargeBoxSerialNumber != nullptr) json.addMemberString("chargeBoxSerialNumber", chargeBoxSerialNumber);
            if (firmwareVersion != nullptr) json.addMemberString("firmwareVersion", firmwareVersion);
            if (iccid != nullptr) json.addMemberString("iccid", iccid);
            if (imsi != nullptr) json.addMemberString("imsi", imsi);
            if (meterType != nullptr) json.addMemberString("meterType", meterType);
            if (meterSerialNumber != nullptr) json.addMemberString("meterSerialNumber", meterSerialNumber);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (status != ChangeAvailabilityResponseStatus::NONE) json.addMemberString("status", ChangeAvailabilityResponseStatusStrings[(size_t)status]);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (status != ChangeConfigurationResponseStatus::NONE) json.addMemberString("status", ChangeConfigurationResponseStatusStrings[(size_t)status]);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (status != ResponseStatus::NONE) json.addMemberString("status", ResponseStatusStrings[(size_t)status]);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALL);
        json.addNumber(this->ocppJmessageId, true);
        json.addString(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (vendorId != nullptr) json.addMemberString("vendorId", vendorId);
            if (messageId != nullptr) json.addMemberString("messageId", messageId);
            if (data != nullptr) json.addMemberString("data", data);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (status != DataTransferResponseStatus::NONE) json.addMemberString("status", DataTransferResponseStatusStrings[(size_t)status]);
            if (data != nullptr) json.addMemberString("data", data);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (configurationKey != nullptr) { json.addMemberArray("configurationKey"); for(size_t i = 0; i < configurationKey_length; ++i) { json.addObject(); configurationKey[i].serializeInto(json); json.endObject(); } json.endArray(); }
            if (unknownKey != nullptr) { json.addMemberArray("unknownKey"); for(size_t i = 0; i < unknownKey_length; ++i) { json.addString(unknownKey[i]); } json.endArray(); }
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
        json.addNumber((int32_t)OcppRpcMessageType::CALL);
        json.addNumber(this->ocppJmessageId, true);
        json.addString(CallActionStrings[(size_t)this->action]);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALL);
        json.addNumber(this->ocppJmessageId, true);
        json.addString(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (connectorId != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("connectorId", connectorId);
            if (transactionId != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("transactionId", transactionId);
            if (meterValue != nullptr) { json.addMemberArray("meterValue"); for(size_t i = 0; i < meterValue_length; ++i) { json.addObject(); meterValue[i].serializeInto(json); json.endObject(); } json.endArray(); }
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
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (status != ResponseStatus::NONE) json.addMemberString("status", ResponseStatusStrings[(size_t)status]);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (status != ResponseStatus::NONE) json.addMemberString("status", ResponseStatusStrings[(size_t)status]);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (status != ResponseStatus::NONE) json.addMemberString("status", ResponseStatusStrings[(size_t)status]);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALL);
        json.addNumber(this->ocppJmessageId, true);
        json.addString(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (connectorId != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("connectorId", connectorId);
            if (idTag != nullptr) json.addMemberString("idTag", idTag);
            if (meterStart != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("meterStart", meterStart);
            if (reservationId != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("reservationId", reservationId);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALL);
        json.addNumber(this->ocppJmessageId, true);
        json.addString(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (connectorId != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("connectorId", connectorId);
            if (errorCode != StatusNotificationErrorCode::NONE) json.addMemberString("errorCode", StatusNotificationErrorCodeStrings[(size_t)errorCode]);
            if (info != nullptr) json.addMemberString("info", info);
            if (status != StatusNotificationStatus::NONE) json.addMemberString("status", StatusNotificationStatusStrings[(size_t)status]);
            if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, json, "timestamp");
            if (vendorId != nullptr) json.addMemberString("vendorId", vendorId);
            if (vendorErrorCode != nullptr) json.addMemberString("vendorErrorCode", vendorErrorCode);
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
        json.addNumber((int32_t)OcppRpcMessageType::CALL);
        json.addNumber(this->ocppJmessageId, true);
        json.addString(CallActionStrings[(size_t)this->action]);
        json.addObject();
            if (idTag != nullptr) json.addMemberString("idTag", idTag);
            if (meterStop != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("meterStop", meterStop);
            if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, json, "timestamp");
            if (transactionId != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("transactionId", transactionId);
            if (reason != StopTransactionReason::NONE) json.addMemberString("reason", StopTransactionReasonStrings[(size_t)reason]);
            if (transactionData != nullptr) { json.addMemberArray("transactionData"); for(size_t i = 0; i < transactionData_length; ++i) { json.addObject(); transactionData[i].serializeInto(json); json.endObject(); } json.endArray(); }
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
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (status != UnlockConnectorResponseStatus::NONE) json.addMemberString("status", UnlockConnectorResponseStatusStrings[(size_t)status]);
        json.endObject();
    json.endArray();

    return json.end();
}

ClearChargingProfileResponse::ClearChargingProfileResponse(const char *call_id,
        ClearChargingProfileResponseStatus status) :
    ICall(CallAction::CLEAR_CHARGING_PROFILE_RESPONSE, call_id),
    status(status)
{}

size_t ClearChargingProfileResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (status != ClearChargingProfileResponseStatus::NONE) json.addMemberString("status", ClearChargingProfileResponseStatusStrings[(size_t)status]);
        json.endObject();
    json.endArray();

    return json.end();
}

GetCompositeScheduleResponse::GetCompositeScheduleResponse(const char *call_id,
        ResponseStatus status,
        int32_t connectorId,
        time_t scheduleStart,
        GetCompositeScheduleResponseChargingSchedule *chargingSchedule) :
    ICall(CallAction::GET_COMPOSITE_SCHEDULE_RESPONSE, call_id),
    status(status),
    connectorId(connectorId),
    scheduleStart(scheduleStart),
    chargingSchedule(chargingSchedule)
{}

size_t GetCompositeScheduleResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (status != ResponseStatus::NONE) json.addMemberString("status", ResponseStatusStrings[(size_t)status]);
            if (connectorId != OCPP_INTEGER_NOT_PASSED) json.addMemberNumber("connectorId", connectorId);
            if (scheduleStart != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(scheduleStart, json, "scheduleStart");
            if (chargingSchedule != nullptr) { json.addMemberObject("chargingSchedule"); chargingSchedule->serializeInto(json); json.endObject(); }
        json.endObject();
    json.endArray();

    return json.end();
}

SetChargingProfileResponse::SetChargingProfileResponse(const char *call_id,
        SetChargingProfileResponseStatus status) :
    ICall(CallAction::SET_CHARGING_PROFILE_RESPONSE, call_id),
    status(status)
{}

size_t SetChargingProfileResponse::serializeJson(char *buf, size_t buf_len) const {
    TFJsonSerializer json{buf, buf_len};
    json.addArray();
        json.addNumber((int32_t)OcppRpcMessageType::CALLRESULT);
        json.addString(this->ocppJcallId);

        json.addObject();
            if (status != SetChargingProfileResponseStatus::NONE) json.addMemberString("status", SetChargingProfileResponseStatusStrings[(size_t)status]);
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
        for(size_t i = 0; i < ARRAY_SIZE(ChargingProfilePurposeStrings); ++i) {
            if (strcmp(var.as<const char *>(), ChargingProfilePurposeStrings[i]) != 0)
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

static CallResponse parseRemoteStartTransactionChargingProfileChargingProfileKind(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingProfileKind: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(ChargingProfileKindStrings); ++i) {
            if (strcmp(var.as<const char *>(), ChargingProfileKindStrings[i]) != 0)
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

static CallResponse parseRemoteStartTransactionChargingProfileRecurrencyKind(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "recurrencyKind: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(RecurrencyKindStrings); ++i) {
            if (strcmp(var.as<const char *>(), RecurrencyKindStrings[i]) != 0)
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
        for(size_t i = 0; i < ARRAY_SIZE(ChargingRateUnitStrings); ++i) {
            if (strcmp(var.as<const char *>(), ChargingRateUnitStrings[i]) != 0)
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
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileChargingProfileKind(obj["chargingProfileKind"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("recurrencyKind")) {
    {
        CallResponse inner_result = parseRemoteStartTransactionChargingProfileRecurrencyKind(obj["recurrencyKind"]);
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

static CallResponse parseClearChargingProfileId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "id: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseClearChargingProfileConnectorId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "connectorId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseClearChargingProfileChargingProfilePurpose(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingProfilePurpose: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(ChargingProfilePurposeStrings); ++i) {
            if (strcmp(var.as<const char *>(), ChargingProfilePurposeStrings[i]) != 0)
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

static CallResponse parseClearChargingProfileStackLevel(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "stackLevel: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseClearChargingProfile(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.containsKey("id")) {
    {
        CallResponse inner_result = parseClearChargingProfileId(obj["id"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("connectorId")) {
    {
        CallResponse inner_result = parseClearChargingProfileConnectorId(obj["connectorId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("chargingProfilePurpose")) {
    {
        CallResponse inner_result = parseClearChargingProfileChargingProfilePurpose(obj["chargingProfilePurpose"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("stackLevel")) {
    {
        CallResponse inner_result = parseClearChargingProfileStackLevel(obj["stackLevel"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "ClearChargingProfile: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseGetCompositeScheduleConnectorId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "connectorId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseGetCompositeScheduleDuration(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "duration: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseGetCompositeScheduleChargingRateUnit(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingRateUnit: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(ChargingRateUnitStrings); ++i) {
            if (strcmp(var.as<const char *>(), ChargingRateUnitStrings[i]) != 0)
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
CallResponse parseGetCompositeSchedule(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("connectorId"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "connectorId: required, but missing"};

    {
        CallResponse inner_result = parseGetCompositeScheduleConnectorId(obj["connectorId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("duration"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "duration: required, but missing"};

    {
        CallResponse inner_result = parseGetCompositeScheduleDuration(obj["duration"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("chargingRateUnit")) {
    {
        CallResponse inner_result = parseGetCompositeScheduleChargingRateUnit(obj["chargingRateUnit"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "GetCompositeSchedule: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseSetChargingProfileConnectorId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "connectorId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingProfileId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingProfileId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesTransactionId(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "transactionId: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesStackLevel(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "stackLevel: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingProfilePurpose(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingProfilePurpose: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(ChargingProfilePurposeStrings); ++i) {
            if (strcmp(var.as<const char *>(), ChargingProfilePurposeStrings[i]) != 0)
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

static CallResponse parseSetChargingProfileCsChargingProfilesChargingProfileKind(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingProfileKind: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(ChargingProfileKindStrings); ++i) {
            if (strcmp(var.as<const char *>(), ChargingProfileKindStrings[i]) != 0)
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

static CallResponse parseSetChargingProfileCsChargingProfilesRecurrencyKind(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "recurrencyKind: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(RecurrencyKindStrings); ++i) {
            if (strcmp(var.as<const char *>(), RecurrencyKindStrings[i]) != 0)
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

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesValidFrom(JsonVariant var) {

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

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesValidTo(JsonVariant var) {

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

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesDuration(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "duration: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesStartSchedule(JsonVariant var) {

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

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingRateUnit(JsonVariant var) {

    if (!var.is<const char *>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingRateUnit: wrong type"};

    {
        bool found = false;
        for(size_t i = 0; i < ARRAY_SIZE(ChargingRateUnitStrings); ++i) {
            if (strcmp(var.as<const char *>(), ChargingRateUnitStrings[i]) != 0)
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

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesStartPeriod(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "startPeriod: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesLimit(JsonVariant var) {

    if (!var.is<float>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "limit: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesNumberPhases(JsonVariant var) {

    if (!var.is<int32_t>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "numberPhases: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntries(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("startPeriod"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "startPeriod: required, but missing"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesStartPeriod(obj["startPeriod"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("limit"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "limit: required, but missing"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesLimit(obj["limit"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("numberPhases")) {
    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesNumberPhases(obj["numberPhases"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "SetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntries: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntry(JsonVariant var) {

    if (!var.is<JsonObject>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingSchedulePeriodEntry: wrong type"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntries(var);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriod(JsonVariant var) {

    if (!var.is<JsonArray>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingSchedulePeriod: wrong type"};

    {
        for(size_t i = 0; i < var.as<JsonArray>().size(); ++i) {
            CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntry(var[i]);
            if (inner_result.result != CallErrorCode::OK)
                return inner_result;
        }
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesMinChargingRate(JsonVariant var) {

    if (!var.is<float>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "minChargingRate: wrong type"};

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntries(JsonObject obj) {
    size_t keys_handled = 0;

    if (obj.containsKey("duration")) {
    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesDuration(obj["duration"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("startSchedule")) {
    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesStartSchedule(obj["startSchedule"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (!obj.containsKey("chargingRateUnit"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingRateUnit: required, but missing"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingRateUnit(obj["chargingRateUnit"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("chargingSchedulePeriod"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingSchedulePeriod: required, but missing"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriod(obj["chargingSchedulePeriod"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("minChargingRate")) {
    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesMinChargingRate(obj["minChargingRate"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "SetChargingProfileCsChargingProfilesEntriesChargingScheduleEntries: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseSetChargingProfileCsChargingProfilesEntriesChargingSchedule(JsonVariant var) {

    if (!var.is<JsonObject>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "chargingSchedule: wrong type"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingScheduleEntries(var);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseSetChargingProfileCsChargingProfilesEntries(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("chargingProfileId"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingProfileId: required, but missing"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingProfileId(obj["chargingProfileId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("transactionId")) {
    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesTransactionId(obj["transactionId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (!obj.containsKey("stackLevel"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "stackLevel: required, but missing"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesStackLevel(obj["stackLevel"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("chargingProfilePurpose"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingProfilePurpose: required, but missing"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingProfilePurpose(obj["chargingProfilePurpose"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("chargingProfileKind"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingProfileKind: required, but missing"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesChargingProfileKind(obj["chargingProfileKind"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    if (obj.containsKey("recurrencyKind")) {
    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesRecurrencyKind(obj["recurrencyKind"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("validFrom")) {
    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesValidFrom(obj["validFrom"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }
    if (obj.containsKey("validTo")) {
    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesValidTo(obj["validTo"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;
    }

    if (!obj.containsKey("chargingSchedule"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "chargingSchedule: required, but missing"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntriesChargingSchedule(obj["chargingSchedule"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "SetChargingProfileCsChargingProfilesEntries: unknown members passed"};
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
static CallResponse parseSetChargingProfileCsChargingProfiles(JsonVariant var) {

    if (!var.is<JsonObject>())
        return CallResponse{CallErrorCode::TypeConstraintViolation, "csChargingProfiles: wrong type"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfilesEntries(var);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}
CallResponse parseSetChargingProfile(JsonObject obj) {
    size_t keys_handled = 0;

    if (!obj.containsKey("connectorId"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "connectorId: required, but missing"};

    {
        CallResponse inner_result = parseSetChargingProfileConnectorId(obj["connectorId"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (!obj.containsKey("csChargingProfiles"))
        return CallResponse{CallErrorCode::OccurenceConstraintViolation, "csChargingProfiles: required, but missing"};

    {
        CallResponse inner_result = parseSetChargingProfileCsChargingProfiles(obj["csChargingProfiles"]);
        if (inner_result.result != CallErrorCode::OK)
            return inner_result;
    }
    ++keys_handled;

    if (obj.size() != keys_handled) {
        return CallResponse{CallErrorCode::FormationViolation, "SetChargingProfile: unknown members passed"};
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

        case CallAction::CLEAR_CHARGING_PROFILE: {
            CallResponse res = parseClearChargingProfile(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleClearChargingProfile(uid, ClearChargingProfileView{obj});
        }

        case CallAction::GET_COMPOSITE_SCHEDULE: {
            CallResponse res = parseGetCompositeSchedule(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleGetCompositeSchedule(uid, GetCompositeScheduleView{obj});
        }

        case CallAction::SET_CHARGING_PROFILE: {
            CallResponse res = parseSetChargingProfile(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleSetChargingProfile(uid, SetChargingProfileView{obj});
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
        case CallAction::TRIGGER_MESSAGE_RESPONSE:
        case CallAction::TRIGGER_MESSAGE:
            return CallResponse{CallErrorCode::NotSupported, "action not supported"};
    }

    SILENCE_GCC_UNREACHABLE();
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

    SILENCE_GCC_UNREACHABLE();
}
