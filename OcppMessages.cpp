#include "OcppMessages.h"

#include "OcppChargePoint.h"
#include "OcppPlatform.h"

extern "C" {
#include "libiso8601/iso8601.h"
}

static bool iso_string_to_unix_timestamp(const char *iso_string, time_t *t) {
    iso8601_time time;
    if (iso8601_parse(iso_string, &time) != 0) {
        return false;
    }
    iso8601_to_time_t(&time, t);
    return true;
}

static void unix_timestamp_to_iso_string(time_t timestamp, JsonObject payload, const char *key) {
    char buf[OCPP_ISO_8601_MAX_LEN] = {0};
    const tm *t = gmtime(&timestamp);

    strftime(buf, ARRAY_SIZE(buf), "%FT%TZ", t);

    payload[key] = buf;
}

static uint32_t next_call_id = 0;

const char *ChangeAvailabilityResponseStatusStrings[] = {
    "Accepted",
    "Rejected",
    "Scheduled"
};

const char *ChangeConfigurationResponseStatusStrings[] = {
    "Accepted",
    "Rejected",
    "RebootRequired",
    "NotSupported"
};

const char *ResponseStatusStrings[] = {
    "Accepted",
    "Rejected"
};

const char *DataTransferResponseStatusStrings[] = {
    "Accepted",
    "Rejected",
    "UnknownMessageId",
    "UnknownVendorId"
};

const char *StatusNotificationErrorCodeStrings[] = {
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

const char *StatusNotificationStatusStrings[] = {
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

const char *StopTransactionReasonStrings[] = {
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

const char *UnlockConnectorResponseStatusStrings[] = {
    "Unlocked",
    "UnlockFailed",
    "NotSupported"
};

const char *ResponseIdTagInfoEntriesStatusStrings[] = {
    "Accepted",
    "Blocked",
    "Expired",
    "Invalid",
    "ConcurrentTx"
};

const char *BootNotificationResponseStatusStrings[] = {
    "Accepted",
    "Pending",
    "Rejected"
};

const char *ChangeAvailabilityTypeStrings[] = {
    "Inoperative",
    "Operative"
};

const char *RemoteStartTransactionChargingProfileEntriesChargingProfilePurposeStrings[] = {
    "ChargePointMaxProfile",
    "TxDefaultProfile",
    "TxProfile"
};

const char *RemoteStartTransactionChargingProfileEntriesChargingProfileKindStrings[] = {
    "Absolute",
    "Recurring",
    "Relative"
};

const char *RemoteStartTransactionChargingProfileEntriesRecurrencyKindStrings[] = {
    "Daily",
    "Weekly"
};

const char *RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingRateUnitStrings[] = {
    "A",
    "W"
};

const char *ResetTypeStrings[] = {
    "Hard",
    "Soft"
};

const char *SampledValueContextStrings[] = {
    "Interruption.Begin",
    "Interruption.End",
    "Sample.Clock",
    "Sample.Periodic",
    "Transaction.Begin",
    "Transaction.End",
    "Trigger",
    "Other"
};

const char *SampledValueFormatStrings[] = {
    "Raw",
    "SignedData"
};

const char *SampledValueMeasurandStrings[] = {
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

const char *SampledValuePhaseStrings[] = {
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

const char *SampledValueLocationStrings[] = {
    "Cable",
    "EV",
    "Inlet",
    "Outlet",
    "Body"
};

const char *MeterValuesMeterValueSampledValueUnitStrings[] = {
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

const char *StopTransactionTransactionDataSampledValueUnitStrings[] = {
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
    "Fahrenheit",
    "Percent"
};


void GetConfigurationResponseConfigurationKey::serializeInto(JsonObject payload) {
        if (key != nullptr) payload["key"] = key;
        payload["readonly"] = readonly;
        if (value != nullptr) payload["value"] = value;
    }

void MeterValuesMeterValue::serializeInto(JsonObject payload) {
        if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, payload, "timestamp");
        if (sampledValue != nullptr) { JsonArray arr = payload.createNestedArray("sampledValue"); for(size_t i = 0; i < sampledValue_length; ++i) { JsonObject obj = arr.createNestedObject(); sampledValue[i].serializeInto(obj); } }
    }

void StopTransactionTransactionData::serializeInto(JsonObject payload) {
        if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, payload, "timestamp");
        if (sampledValue != nullptr) { JsonArray arr = payload.createNestedArray("sampledValue"); for(size_t i = 0; i < sampledValue_length; ++i) { JsonObject obj = arr.createNestedObject(); sampledValue[i].serializeInto(obj); } }
    }

void MeterValuesMeterValueSampledValue::serializeInto(JsonObject payload) {
        if (value != nullptr) payload["value"] = value;
        if (context != SampledValueContext::NONE) payload["context"] = SampledValueContextStrings[(size_t)context];
        if (format != SampledValueFormat::NONE) payload["format"] = SampledValueFormatStrings[(size_t)format];
        if (measurand != SampledValueMeasurand::NONE) payload["measurand"] = SampledValueMeasurandStrings[(size_t)measurand];
        if (phase != SampledValuePhase::NONE) payload["phase"] = SampledValuePhaseStrings[(size_t)phase];
        if (location != SampledValueLocation::NONE) payload["location"] = SampledValueLocationStrings[(size_t)location];
        if (unit != MeterValuesMeterValueSampledValueUnit::NONE) payload["unit"] = MeterValuesMeterValueSampledValueUnitStrings[(size_t)unit];
    }

void StopTransactionTransactionDataSampledValue::serializeInto(JsonObject payload) {
        if (value != nullptr) payload["value"] = value;
        if (context != SampledValueContext::NONE) payload["context"] = SampledValueContextStrings[(size_t)context];
        if (format != SampledValueFormat::NONE) payload["format"] = SampledValueFormatStrings[(size_t)format];
        if (measurand != SampledValueMeasurand::NONE) payload["measurand"] = SampledValueMeasurandStrings[(size_t)measurand];
        if (phase != SampledValuePhase::NONE) payload["phase"] = SampledValuePhaseStrings[(size_t)phase];
        if (location != SampledValueLocation::NONE) payload["location"] = SampledValueLocationStrings[(size_t)location];
        if (unit != StopTransactionTransactionDataSampledValueUnit::NONE) payload["unit"] = StopTransactionTransactionDataSampledValueUnitStrings[(size_t)unit];
    }



DynamicJsonDocument Authorize(const char idTag[21]) {
    if (idTag == nullptr) { platform_printfln("Required idTag missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALL_JSON_SIZE("Authorize")
        + JSON_OBJECT_SIZE(1)
        + 2000
    };

    result.add((int32_t)OcppRpcMessageType::CALL);
    result.add(std::to_string(next_call_id));
    result.add("Authorize");
    JsonObject payload = result.createNestedObject();

    if (idTag != nullptr) payload["idTag"] = idTag;

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument BootNotification(const char chargePointVendor[21],
        const char chargePointModel[21],
        const char chargePointSerialNumber[26],
        const char chargeBoxSerialNumber[26],
        const char firmwareVersion[51],
        const char iccid[21],
        const char imsi[21],
        const char meterType[26],
        const char meterSerialNumber[26]) {
    if (chargePointVendor == nullptr) { platform_printfln("Required chargePointVendor missing."); return DynamicJsonDocument{0}; }
    if (chargePointModel == nullptr) { platform_printfln("Required chargePointModel missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALL_JSON_SIZE("BootNotification")
        + JSON_OBJECT_SIZE(9)
        + 18000
    };

    result.add((int32_t)OcppRpcMessageType::CALL);
    result.add(std::to_string(next_call_id));
    result.add("BootNotification");
    JsonObject payload = result.createNestedObject();

    if (chargePointVendor != nullptr) payload["chargePointVendor"] = chargePointVendor;
    if (chargePointModel != nullptr) payload["chargePointModel"] = chargePointModel;
    if (chargePointSerialNumber != nullptr) payload["chargePointSerialNumber"] = chargePointSerialNumber;
    if (chargeBoxSerialNumber != nullptr) payload["chargeBoxSerialNumber"] = chargeBoxSerialNumber;
    if (firmwareVersion != nullptr) payload["firmwareVersion"] = firmwareVersion;
    if (iccid != nullptr) payload["iccid"] = iccid;
    if (imsi != nullptr) payload["imsi"] = imsi;
    if (meterType != nullptr) payload["meterType"] = meterType;
    if (meterSerialNumber != nullptr) payload["meterSerialNumber"] = meterSerialNumber;

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument ChangeAvailabilityResponse(const char *call_id,
        ChangeAvailabilityResponseStatus status) {
    if (status == ChangeAvailabilityResponseStatus::NONE) { platform_printfln("Required status missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALLRESULT_JSON_SIZE("ChangeAvailabilityResponse")
        + JSON_OBJECT_SIZE(1)
        + 2000
    };

    result.add((int32_t)OcppRpcMessageType::CALLRESULT);
    result.add(call_id);

    JsonObject payload = result.createNestedObject();

    if (status != ChangeAvailabilityResponseStatus::NONE) payload["status"] = ChangeAvailabilityResponseStatusStrings[(size_t)status];

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument ChangeConfigurationResponse(const char *call_id,
        ChangeConfigurationResponseStatus status) {
    if (status == ChangeConfigurationResponseStatus::NONE) { platform_printfln("Required status missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALLRESULT_JSON_SIZE("ChangeConfigurationResponse")
        + JSON_OBJECT_SIZE(1)
        + 2000
    };

    result.add((int32_t)OcppRpcMessageType::CALLRESULT);
    result.add(call_id);

    JsonObject payload = result.createNestedObject();

    if (status != ChangeConfigurationResponseStatus::NONE) payload["status"] = ChangeConfigurationResponseStatusStrings[(size_t)status];

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument ClearCacheResponse(const char *call_id,
        ResponseStatus status) {
    if (status == ResponseStatus::NONE) { platform_printfln("Required status missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALLRESULT_JSON_SIZE("ClearCacheResponse")
        + JSON_OBJECT_SIZE(1)
        + 2000
    };

    result.add((int32_t)OcppRpcMessageType::CALLRESULT);
    result.add(call_id);

    JsonObject payload = result.createNestedObject();

    if (status != ResponseStatus::NONE) payload["status"] = ResponseStatusStrings[(size_t)status];

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument DataTransfer(const char vendorId[256],
        const char messageId[51],
        const char *data) {
    if (vendorId == nullptr) { platform_printfln("Required vendorId missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALL_JSON_SIZE("DataTransfer")
        + JSON_OBJECT_SIZE(3)
        + 6000
    };

    result.add((int32_t)OcppRpcMessageType::CALL);
    result.add(std::to_string(next_call_id));
    result.add("DataTransfer");
    JsonObject payload = result.createNestedObject();

    if (vendorId != nullptr) payload["vendorId"] = vendorId;
    if (messageId != nullptr) payload["messageId"] = messageId;
    if (data != nullptr) payload["data"] = data;

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument DataTransferResponse(const char *call_id,
        DataTransferResponseStatus status,
        const char *data) {
    if (status == DataTransferResponseStatus::NONE) { platform_printfln("Required status missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALLRESULT_JSON_SIZE("DataTransferResponse")
        + JSON_OBJECT_SIZE(2)
        + 4000
    };

    result.add((int32_t)OcppRpcMessageType::CALLRESULT);
    result.add(call_id);

    JsonObject payload = result.createNestedObject();

    if (status != DataTransferResponseStatus::NONE) payload["status"] = DataTransferResponseStatusStrings[(size_t)status];
    if (data != nullptr) payload["data"] = data;

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument GetConfigurationResponse(const char *call_id,
        GetConfigurationResponseConfigurationKey *configurationKey, size_t configurationKey_length,
        const char **unknownKey, size_t unknownKey_length) {

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALLRESULT_JSON_SIZE("GetConfigurationResponse")
        + JSON_OBJECT_SIZE(2)
        + 4000
    };

    result.add((int32_t)OcppRpcMessageType::CALLRESULT);
    result.add(call_id);

    JsonObject payload = result.createNestedObject();

    if (configurationKey != nullptr) { JsonArray arr = payload.createNestedArray("configurationKey"); for(size_t i = 0; i < configurationKey_length; ++i) { JsonObject obj = arr.createNestedObject(); configurationKey[i].serializeInto(obj); } }
    if (unknownKey != nullptr) { JsonArray arr = payload.createNestedArray("unknownKey"); for(size_t i = 0; i < unknownKey_length; ++i) { arr.add(unknownKey[i]); } }

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument Heartbeat() {

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALL_JSON_SIZE("Heartbeat")
        + JSON_OBJECT_SIZE(0)
        + 0
    };

    result.add((int32_t)OcppRpcMessageType::CALL);
    result.add(std::to_string(next_call_id));
    result.add("Heartbeat");
    result.createNestedObject();

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument MeterValues(int32_t connectorId,
        MeterValuesMeterValue *meterValue, size_t meterValue_length,
        int32_t transactionId) {
    if (connectorId == OCPP_INTEGER_NOT_PASSED) { platform_printfln("Required connectorId missing."); return DynamicJsonDocument{0}; }
    if (meterValue == nullptr) { platform_printfln("Required meterValue missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALL_JSON_SIZE("MeterValues")
        + JSON_OBJECT_SIZE(3)
        + 6000
    };

    result.add((int32_t)OcppRpcMessageType::CALL);
    result.add(std::to_string(next_call_id));
    result.add("MeterValues");
    JsonObject payload = result.createNestedObject();

    if (connectorId != OCPP_INTEGER_NOT_PASSED) payload["connectorId"] = connectorId;
    if (transactionId != OCPP_INTEGER_NOT_PASSED) payload["transactionId"] = transactionId;
    if (meterValue != nullptr) { JsonArray arr = payload.createNestedArray("meterValue"); for(size_t i = 0; i < meterValue_length; ++i) { JsonObject obj = arr.createNestedObject(); meterValue[i].serializeInto(obj); } }

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument RemoteStartTransactionResponse(const char *call_id,
        ResponseStatus status) {
    if (status == ResponseStatus::NONE) { platform_printfln("Required status missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALLRESULT_JSON_SIZE("RemoteStartTransactionResponse")
        + JSON_OBJECT_SIZE(1)
        + 2000
    };

    result.add((int32_t)OcppRpcMessageType::CALLRESULT);
    result.add(call_id);

    JsonObject payload = result.createNestedObject();

    if (status != ResponseStatus::NONE) payload["status"] = ResponseStatusStrings[(size_t)status];

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument RemoteStopTransactionResponse(const char *call_id,
        ResponseStatus status) {
    if (status == ResponseStatus::NONE) { platform_printfln("Required status missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALLRESULT_JSON_SIZE("RemoteStopTransactionResponse")
        + JSON_OBJECT_SIZE(1)
        + 2000
    };

    result.add((int32_t)OcppRpcMessageType::CALLRESULT);
    result.add(call_id);

    JsonObject payload = result.createNestedObject();

    if (status != ResponseStatus::NONE) payload["status"] = ResponseStatusStrings[(size_t)status];

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument ResetResponse(const char *call_id,
        ResponseStatus status) {
    if (status == ResponseStatus::NONE) { platform_printfln("Required status missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALLRESULT_JSON_SIZE("ResetResponse")
        + JSON_OBJECT_SIZE(1)
        + 2000
    };

    result.add((int32_t)OcppRpcMessageType::CALLRESULT);
    result.add(call_id);

    JsonObject payload = result.createNestedObject();

    if (status != ResponseStatus::NONE) payload["status"] = ResponseStatusStrings[(size_t)status];

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument StartTransaction(int32_t connectorId,
        const char idTag[21],
        int32_t meterStart,
        time_t timestamp,
        int32_t reservationId) {
    if (connectorId == OCPP_INTEGER_NOT_PASSED) { platform_printfln("Required connectorId missing."); return DynamicJsonDocument{0}; }
    if (idTag == nullptr) { platform_printfln("Required idTag missing."); return DynamicJsonDocument{0}; }
    if (meterStart == OCPP_INTEGER_NOT_PASSED) { platform_printfln("Required meterStart missing."); return DynamicJsonDocument{0}; }
    if (timestamp == OCPP_DATETIME_NOT_PASSED) { platform_printfln("Required timestamp missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALL_JSON_SIZE("StartTransaction")
        + JSON_OBJECT_SIZE(5)
        + 10000
    };

    result.add((int32_t)OcppRpcMessageType::CALL);
    result.add(std::to_string(next_call_id));
    result.add("StartTransaction");
    JsonObject payload = result.createNestedObject();

    if (connectorId != OCPP_INTEGER_NOT_PASSED) payload["connectorId"] = connectorId;
    if (idTag != nullptr) payload["idTag"] = idTag;
    if (meterStart != OCPP_INTEGER_NOT_PASSED) payload["meterStart"] = meterStart;
    if (reservationId != OCPP_INTEGER_NOT_PASSED) payload["reservationId"] = reservationId;
    if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, payload, "timestamp");

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument StatusNotification(int32_t connectorId,
        StatusNotificationErrorCode errorCode,
        StatusNotificationStatus status,
        const char info[51],
        time_t timestamp,
        const char vendorId[256],
        const char vendorErrorCode[51]) {
    if (connectorId == OCPP_INTEGER_NOT_PASSED) { platform_printfln("Required connectorId missing."); return DynamicJsonDocument{0}; }
    if (errorCode == StatusNotificationErrorCode::NONE) { platform_printfln("Required errorCode missing."); return DynamicJsonDocument{0}; }
    if (status == StatusNotificationStatus::NONE) { platform_printfln("Required status missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALL_JSON_SIZE("StatusNotification")
        + JSON_OBJECT_SIZE(7)
        + 14000
    };

    result.add((int32_t)OcppRpcMessageType::CALL);
    result.add(std::to_string(next_call_id));
    result.add("StatusNotification");
    JsonObject payload = result.createNestedObject();

    if (connectorId != OCPP_INTEGER_NOT_PASSED) payload["connectorId"] = connectorId;
    if (errorCode != StatusNotificationErrorCode::NONE) payload["errorCode"] = StatusNotificationErrorCodeStrings[(size_t)errorCode];
    if (info != nullptr) payload["info"] = info;
    if (status != StatusNotificationStatus::NONE) payload["status"] = StatusNotificationStatusStrings[(size_t)status];
    if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, payload, "timestamp");
    if (vendorId != nullptr) payload["vendorId"] = vendorId;
    if (vendorErrorCode != nullptr) payload["vendorErrorCode"] = vendorErrorCode;

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument StopTransaction(int32_t meterStop,
        time_t timestamp,
        int32_t transactionId,
        const char idTag[21],
        StopTransactionReason reason,
        StopTransactionTransactionData *transactionData, size_t transactionData_length) {
    if (meterStop == OCPP_INTEGER_NOT_PASSED) { platform_printfln("Required meterStop missing."); return DynamicJsonDocument{0}; }
    if (timestamp == OCPP_DATETIME_NOT_PASSED) { platform_printfln("Required timestamp missing."); return DynamicJsonDocument{0}; }
    if (transactionId == OCPP_INTEGER_NOT_PASSED) { platform_printfln("Required transactionId missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALL_JSON_SIZE("StopTransaction")
        + JSON_OBJECT_SIZE(6)
        + 12000
    };

    result.add((int32_t)OcppRpcMessageType::CALL);
    result.add(std::to_string(next_call_id));
    result.add("StopTransaction");
    JsonObject payload = result.createNestedObject();

    if (idTag != nullptr) payload["idTag"] = idTag;
    if (meterStop != OCPP_INTEGER_NOT_PASSED) payload["meterStop"] = meterStop;
    if (timestamp != OCPP_DATETIME_NOT_PASSED) unix_timestamp_to_iso_string(timestamp, payload, "timestamp");
    if (transactionId != OCPP_INTEGER_NOT_PASSED) payload["transactionId"] = transactionId;
    if (reason != StopTransactionReason::NONE) payload["reason"] = StopTransactionReasonStrings[(size_t)reason];
    if (transactionData != nullptr) { JsonArray arr = payload.createNestedArray("transactionData"); for(size_t i = 0; i < transactionData_length; ++i) { JsonObject obj = arr.createNestedObject(); transactionData[i].serializeInto(obj); } }

    result.shrinkToFit();
    return result;
}

DynamicJsonDocument UnlockConnectorResponse(const char *call_id,
        UnlockConnectorResponseStatus status) {
    if (status == UnlockConnectorResponseStatus::NONE) { platform_printfln("Required status missing."); return DynamicJsonDocument{0}; }

    DynamicJsonDocument result = DynamicJsonDocument{
        OCPP_CALLRESULT_JSON_SIZE("UnlockConnectorResponse")
        + JSON_OBJECT_SIZE(1)
        + 2000
    };

    result.add((int32_t)OcppRpcMessageType::CALLRESULT);
    result.add(call_id);

    JsonObject payload = result.createNestedObject();

    if (status != UnlockConnectorResponseStatus::NONE) payload["status"] = UnlockConnectorResponseStatusStrings[(size_t)status];

    result.shrinkToFit();
    return result;
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

const char *CallActionStrings[] = {
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

CallResponse callResultHandler(uint32_t message_id, CallAction resultTo, JsonObject obj, OcppChargePoint *cp) {

    switch(resultTo) {
        case CallAction::AUTHORIZE: {
            CallResponse res = parseAuthorizeResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleAuthorizeResponse(message_id, AuthorizeResponseView{obj});
        }

        case CallAction::BOOT_NOTIFICATION: {
            CallResponse res = parseBootNotificationResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleBootNotificationResponse(message_id, BootNotificationResponseView{obj});
        }

        case CallAction::DATA_TRANSFER: {
            CallResponse res = parseDataTransferResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleDataTransferResponse(message_id, DataTransferResponseView{obj});
        }

        case CallAction::HEARTBEAT: {
            CallResponse res = parseHeartbeatResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleHeartbeatResponse(message_id, HeartbeatResponseView{obj});
        }

        case CallAction::METER_VALUES: {
            CallResponse res = parseMeterValuesResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleMeterValuesResponse(message_id, MeterValuesResponseView{obj});
        }

        case CallAction::START_TRANSACTION: {
            CallResponse res = parseStartTransactionResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleStartTransactionResponse(message_id, StartTransactionResponseView{obj});
        }

        case CallAction::STATUS_NOTIFICATION: {
            CallResponse res = parseStatusNotificationResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleStatusNotificationResponse(message_id, StatusNotificationResponseView{obj});
        }

        case CallAction::STOP_TRANSACTION: {
            CallResponse res = parseStopTransactionResponse(obj);
            if (res.result != CallErrorCode::OK)
                return res;

            return cp->handleStopTransactionResponse(message_id, StopTransactionResponseView{obj});
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
