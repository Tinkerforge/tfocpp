#pragma once

#include "OcppTypes.h"

class Ocpp;

#define OCPP_INTEGER_NOT_PASSED INT32_MAX
#define OCPP_DATETIME_NOT_PASSED 0

extern const char *ChangeAvailabilityResponseStatusStrings[];

enum class ChangeAvailabilityResponseStatus {
    ACCEPTED,
    REJECTED,
    SCHEDULED,
    NONE
};

extern const char *ChangeConfigurationResponseStatusStrings[];

enum class ChangeConfigurationResponseStatus {
    ACCEPTED,
    REJECTED,
    REBOOT_REQUIRED,
    NOT_SUPPORTED,
    NONE
};

extern const char *ClearCacheResponseStatusStrings[];

enum class ClearCacheResponseStatus {
    ACCEPTED,
    REJECTED,
    NONE
};

extern const char *DataTransferResponseStatusStrings[];

enum class DataTransferResponseStatus {
    ACCEPTED,
    REJECTED,
    UNKNOWN_MESSAGE_ID,
    UNKNOWN_VENDOR_ID,
    NONE
};

extern const char *RemoteStartTransactionResponseStatusStrings[];

enum class RemoteStartTransactionResponseStatus {
    ACCEPTED,
    REJECTED,
    NONE
};

extern const char *RemoteStopTransactionResponseStatusStrings[];

enum class RemoteStopTransactionResponseStatus {
    ACCEPTED,
    REJECTED,
    NONE
};

extern const char *ResetResponseStatusStrings[];

enum class ResetResponseStatus {
    ACCEPTED,
    REJECTED,
    NONE
};

extern const char *StatusNotificationErrorCodeStrings[];

enum class StatusNotificationErrorCode {
    CONNECTOR_LOCK_FAILURE,
    EV_COMMUNICATION_ERROR,
    GROUND_FAILURE,
    HIGH_TEMPERATURE,
    INTERNAL_ERROR,
    LOCAL_LIST_CONFLICT,
    NO_ERROR,
    OTHER_ERROR,
    OVER_CURRENT_FAILURE,
    POWER_METER_FAILURE,
    POWER_SWITCH_FAILURE,
    READER_FAILURE,
    RESET_FAILURE,
    UNDER_VOLTAGE,
    OVER_VOLTAGE,
    WEAK_SIGNAL,
    NONE
};

extern const char *StatusNotificationStatusStrings[];

enum class StatusNotificationStatus {
    AVAILABLE,
    PREPARING,
    CHARGING,
    SUSPENDED_EVSE,
    SUSPENDED_EV,
    FINISHING,
    RESERVED,
    UNAVAILABLE,
    FAULTED,
    NONE
};

extern const char *StopTransactionReasonStrings[];

enum class StopTransactionReason {
    EMERGENCY_STOP,
    EV_DISCONNECTED,
    HARD_RESET,
    LOCAL,
    OTHER,
    POWER_LOSS,
    REBOOT,
    REMOTE,
    SOFT_RESET,
    UNLOCK_COMMAND,
    DE_AUTHORIZED,
    NONE
};

extern const char *UnlockConnectorResponseStatusStrings[];

enum class UnlockConnectorResponseStatus {
    UNLOCKED,
    UNLOCK_FAILED,
    NOT_SUPPORTED,
    NONE
};

extern const char *AuthorizeResponseIdTagInfoEntriesStatusStrings[];

enum class AuthorizeResponseIdTagInfoEntriesStatus {
    ACCEPTED,
    BLOCKED,
    EXPIRED,
    INVALID,
    CONCURRENT_TX
};

extern const char *BootNotificationResponseStatusStrings[];

enum class BootNotificationResponseStatus {
    ACCEPTED,
    PENDING,
    REJECTED
};

extern const char *ChangeAvailabilityTypeStrings[];

enum class ChangeAvailabilityType {
    INOPERATIVE,
    OPERATIVE
};

extern const char *RemoteStartTransactionChargingProfileEntriesChargingProfilePurposeStrings[];

enum class RemoteStartTransactionChargingProfileEntriesChargingProfilePurpose {
    CHARGE_POINT_MAX_PROFILE,
    TX_DEFAULT_PROFILE,
    TX_PROFILE
};

extern const char *RemoteStartTransactionChargingProfileEntriesChargingProfileKindStrings[];

enum class RemoteStartTransactionChargingProfileEntriesChargingProfileKind {
    ABSOLUTE,
    RECURRING,
    RELATIVE
};

extern const char *RemoteStartTransactionChargingProfileEntriesRecurrencyKindStrings[];

enum class RemoteStartTransactionChargingProfileEntriesRecurrencyKind {
    DAILY,
    WEEKLY
};

extern const char *RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingRateUnitStrings[];

enum class RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingRateUnit {
    A,
    W
};

extern const char *ResetTypeStrings[];

enum class ResetType {
    HARD,
    SOFT
};

extern const char *StartTransactionResponseIdTagInfoEntriesStatusStrings[];

enum class StartTransactionResponseIdTagInfoEntriesStatus {
    ACCEPTED,
    BLOCKED,
    EXPIRED,
    INVALID,
    CONCURRENT_TX
};

extern const char *StopTransactionResponseIdTagInfoEntriesStatusStrings[];

enum class StopTransactionResponseIdTagInfoEntriesStatus {
    ACCEPTED,
    BLOCKED,
    EXPIRED,
    INVALID,
    CONCURRENT_TX
};

extern const char *MeterValuesMeterValueSampledValueContextStrings[];

enum class MeterValuesMeterValueSampledValueContext {
    INTERRUPTION_BEGIN,
    INTERRUPTION_END,
    SAMPLE_CLOCK,
    SAMPLE_PERIODIC,
    TRANSACTION_BEGIN,
    TRANSACTION_END,
    TRIGGER,
    OTHER,
    NONE
};

extern const char *MeterValuesMeterValueSampledValueFormatStrings[];

enum class MeterValuesMeterValueSampledValueFormat {
    RAW,
    SIGNED_DATA,
    NONE
};

extern const char *MeterValuesMeterValueSampledValueMeasurandStrings[];

enum class MeterValuesMeterValueSampledValueMeasurand {
    ENERGY_ACTIVE_EXPORT_REGISTER,
    ENERGY_ACTIVE_IMPORT_REGISTER,
    ENERGY_REACTIVE_EXPORT_REGISTER,
    ENERGY_REACTIVE_IMPORT_REGISTER,
    ENERGY_ACTIVE_EXPORT_INTERVAL,
    ENERGY_ACTIVE_IMPORT_INTERVAL,
    ENERGY_REACTIVE_EXPORT_INTERVAL,
    ENERGY_REACTIVE_IMPORT_INTERVAL,
    POWER_ACTIVE_EXPORT,
    POWER_ACTIVE_IMPORT,
    POWER_OFFERED,
    POWER_REACTIVE_EXPORT,
    POWER_REACTIVE_IMPORT,
    POWER_FACTOR,
    CURRENT_IMPORT,
    CURRENT_EXPORT,
    CURRENT_OFFERED,
    VOLTAGE,
    FREQUENCY,
    TEMPERATURE,
    SO_C,
    RPM,
    NONE
};

extern const char *MeterValuesMeterValueSampledValuePhaseStrings[];

enum class MeterValuesMeterValueSampledValuePhase {
    L1,
    L2,
    L3,
    N,
    L1_N,
    L2_N,
    L3_N,
    L1_L2,
    L2_L3,
    L3_L1,
    NONE
};

extern const char *MeterValuesMeterValueSampledValueLocationStrings[];

enum class MeterValuesMeterValueSampledValueLocation {
    CABLE,
    EV,
    INLET,
    OUTLET,
    BODY,
    NONE
};

extern const char *MeterValuesMeterValueSampledValueUnitStrings[];

enum class MeterValuesMeterValueSampledValueUnit {
    WH,
    K_WH,
    VARH,
    KVARH,
    W,
    K_W,
    VA,
    K_VA,
    VAR,
    KVAR,
    A,
    V,
    K,
    CELCIUS,
    CELSIUS,
    FAHRENHEIT,
    PERCENT,
    NONE
};

extern const char *StopTransactionTransactionDataSampledValueContextStrings[];

enum class StopTransactionTransactionDataSampledValueContext {
    INTERRUPTION_BEGIN,
    INTERRUPTION_END,
    SAMPLE_CLOCK,
    SAMPLE_PERIODIC,
    TRANSACTION_BEGIN,
    TRANSACTION_END,
    TRIGGER,
    OTHER,
    NONE
};

extern const char *StopTransactionTransactionDataSampledValueFormatStrings[];

enum class StopTransactionTransactionDataSampledValueFormat {
    RAW,
    SIGNED_DATA,
    NONE
};

extern const char *StopTransactionTransactionDataSampledValueMeasurandStrings[];

enum class StopTransactionTransactionDataSampledValueMeasurand {
    ENERGY_ACTIVE_EXPORT_REGISTER,
    ENERGY_ACTIVE_IMPORT_REGISTER,
    ENERGY_REACTIVE_EXPORT_REGISTER,
    ENERGY_REACTIVE_IMPORT_REGISTER,
    ENERGY_ACTIVE_EXPORT_INTERVAL,
    ENERGY_ACTIVE_IMPORT_INTERVAL,
    ENERGY_REACTIVE_EXPORT_INTERVAL,
    ENERGY_REACTIVE_IMPORT_INTERVAL,
    POWER_ACTIVE_EXPORT,
    POWER_ACTIVE_IMPORT,
    POWER_OFFERED,
    POWER_REACTIVE_EXPORT,
    POWER_REACTIVE_IMPORT,
    POWER_FACTOR,
    CURRENT_IMPORT,
    CURRENT_EXPORT,
    CURRENT_OFFERED,
    VOLTAGE,
    FREQUENCY,
    TEMPERATURE,
    SO_C,
    RPM,
    NONE
};

extern const char *StopTransactionTransactionDataSampledValuePhaseStrings[];

enum class StopTransactionTransactionDataSampledValuePhase {
    L1,
    L2,
    L3,
    N,
    L1_N,
    L2_N,
    L3_N,
    L1_L2,
    L2_L3,
    L3_L1,
    NONE
};

extern const char *StopTransactionTransactionDataSampledValueLocationStrings[];

enum class StopTransactionTransactionDataSampledValueLocation {
    CABLE,
    EV,
    INLET,
    OUTLET,
    BODY,
    NONE
};

extern const char *StopTransactionTransactionDataSampledValueUnitStrings[];

enum class StopTransactionTransactionDataSampledValueUnit {
    WH,
    K_WH,
    VARH,
    KVARH,
    W,
    K_W,
    VA,
    K_VA,
    VAR,
    KVAR,
    A,
    V,
    K,
    CELCIUS,
    FAHRENHEIT,
    PERCENT,
    NONE
};


struct UnlockConnectorView {
    JsonObject _obj;

    int32_t connectorId() {

        return _obj["connectorId"].as<int32_t>();
    }

};

struct StopTransactionResponseIdTagInfoEntriesView {
    JsonObject _obj;

    Opt<time_t> expiryDate() {
        if (!_obj.containsKey("expiryDate"))
                return {false};

        return _obj["expiryDate"].as<time_t>();
    }

    Opt<const char *> parentIdTag() {
        if (!_obj.containsKey("parentIdTag"))
                return {false};

        return _obj["parentIdTag"].as<const char *>();
    }

    StopTransactionResponseIdTagInfoEntriesStatus status() {

        return (StopTransactionResponseIdTagInfoEntriesStatus)_obj["status"].as<size_t>();
    }

};

struct StopTransactionResponseView {
    JsonObject _obj;

    Opt<StopTransactionResponseIdTagInfoEntriesView> idTagInfo() {
        if (!_obj.containsKey("idTagInfo"))
                return {false};

        return Opt<StopTransactionResponseIdTagInfoEntriesView>{_obj["idTagInfo"].as<JsonObject>()};
    }

};

struct StatusNotificationResponseView {
    JsonObject _obj;

};

struct StartTransactionResponseIdTagInfoEntriesView {
    JsonObject _obj;

    Opt<time_t> expiryDate() {
        if (!_obj.containsKey("expiryDate"))
                return {false};

        return _obj["expiryDate"].as<time_t>();
    }

    Opt<const char *> parentIdTag() {
        if (!_obj.containsKey("parentIdTag"))
                return {false};

        return _obj["parentIdTag"].as<const char *>();
    }

    StartTransactionResponseIdTagInfoEntriesStatus status() {

        return (StartTransactionResponseIdTagInfoEntriesStatus)_obj["status"].as<size_t>();
    }

};

struct StartTransactionResponseView {
    JsonObject _obj;

    StartTransactionResponseIdTagInfoEntriesView idTagInfo() {

        return StartTransactionResponseIdTagInfoEntriesView{_obj["idTagInfo"].as<JsonObject>()};
    }

    int32_t transactionId() {

        return _obj["transactionId"].as<int32_t>();
    }

};

struct ResetView {
    JsonObject _obj;

    ResetType type() {

        return (ResetType)_obj["type"].as<size_t>();
    }

};

struct RemoteStopTransactionView {
    JsonObject _obj;

    int32_t transactionId() {

        return _obj["transactionId"].as<int32_t>();
    }

};

struct RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesView {
    JsonObject _obj;

    int32_t startPeriod() {

        return _obj["startPeriod"].as<int32_t>();
    }

    float limit() {

        return _obj["limit"].as<float>();
    }

    Opt<int32_t> numberPhases() {
        if (!_obj.containsKey("numberPhases"))
                return {false};

        return _obj["numberPhases"].as<int32_t>();
    }

};

struct RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesView {
    JsonObject _obj;

    Opt<int32_t> duration() {
        if (!_obj.containsKey("duration"))
                return {false};

        return _obj["duration"].as<int32_t>();
    }

    Opt<time_t> startSchedule() {
        if (!_obj.containsKey("startSchedule"))
                return {false};

        return _obj["startSchedule"].as<time_t>();
    }

    RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingRateUnit chargingRateUnit() {

        return (RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingRateUnit)_obj["chargingRateUnit"].as<size_t>();
    }

    size_t chargingSchedulePeriod_count() {

        return _obj["chargingSchedulePeriod"].size();
    }

    RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesView chargingSchedulePeriod(size_t i) {

        return RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesView{_obj["chargingSchedulePeriod"][i]};
    }

    Opt<float> minChargingRate() {
        if (!_obj.containsKey("minChargingRate"))
                return {false};

        return _obj["minChargingRate"].as<float>();
    }

};

struct RemoteStartTransactionChargingProfileEntriesView {
    JsonObject _obj;

    int32_t chargingProfileId() {

        return _obj["chargingProfileId"].as<int32_t>();
    }

    Opt<int32_t> transactionId() {
        if (!_obj.containsKey("transactionId"))
                return {false};

        return _obj["transactionId"].as<int32_t>();
    }

    int32_t stackLevel() {

        return _obj["stackLevel"].as<int32_t>();
    }

    RemoteStartTransactionChargingProfileEntriesChargingProfilePurpose chargingProfilePurpose() {

        return (RemoteStartTransactionChargingProfileEntriesChargingProfilePurpose)_obj["chargingProfilePurpose"].as<size_t>();
    }

    RemoteStartTransactionChargingProfileEntriesChargingProfileKind chargingProfileKind() {

        return (RemoteStartTransactionChargingProfileEntriesChargingProfileKind)_obj["chargingProfileKind"].as<size_t>();
    }

    Opt<RemoteStartTransactionChargingProfileEntriesRecurrencyKind> recurrencyKind() {
        if (!_obj.containsKey("recurrencyKind"))
                return {false};

        return (Opt<RemoteStartTransactionChargingProfileEntriesRecurrencyKind>)_obj["recurrencyKind"].as<size_t>();
    }

    Opt<time_t> validFrom() {
        if (!_obj.containsKey("validFrom"))
                return {false};

        return _obj["validFrom"].as<time_t>();
    }

    Opt<time_t> validTo() {
        if (!_obj.containsKey("validTo"))
                return {false};

        return _obj["validTo"].as<time_t>();
    }

    RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesView chargingSchedule() {

        return RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesView{_obj["chargingSchedule"].as<JsonObject>()};
    }

};

struct RemoteStartTransactionView {
    JsonObject _obj;

    Opt<int32_t> connectorId() {
        if (!_obj.containsKey("connectorId"))
                return {false};

        return _obj["connectorId"].as<int32_t>();
    }

    const char * idTag() {

        return _obj["idTag"].as<const char *>();
    }

    Opt<RemoteStartTransactionChargingProfileEntriesView> chargingProfile() {
        if (!_obj.containsKey("chargingProfile"))
                return {false};

        return Opt<RemoteStartTransactionChargingProfileEntriesView>{_obj["chargingProfile"].as<JsonObject>()};
    }

};

struct MeterValuesResponseView {
    JsonObject _obj;

};

struct HeartbeatResponseView {
    JsonObject _obj;

    time_t currentTime() {

        return _obj["currentTime"].as<time_t>();
    }

};

struct GetConfigurationView {
    JsonObject _obj;

    size_t key_count() {

        return _obj["key"].size();
    }

    Opt<const char *> key(size_t i) {
        if (!_obj.containsKey("key"))
                return {false};

        return _obj["key"][i].as<const char *>();
    }

};

struct DataTransferResponseView {
    JsonObject _obj;

    DataTransferResponseStatus status() {

        return (DataTransferResponseStatus)_obj["status"].as<size_t>();
    }

    Opt<const char *> data() {
        if (!_obj.containsKey("data"))
                return {false};

        return _obj["data"].as<const char *>();
    }

};

struct DataTransferView {
    JsonObject _obj;

    const char * vendorId() {

        return _obj["vendorId"].as<const char *>();
    }

    Opt<const char *> messageId() {
        if (!_obj.containsKey("messageId"))
                return {false};

        return _obj["messageId"].as<const char *>();
    }

    Opt<const char *> data() {
        if (!_obj.containsKey("data"))
                return {false};

        return _obj["data"].as<const char *>();
    }

};

struct ClearCacheView {
    JsonObject _obj;

};

struct ChangeConfigurationView {
    JsonObject _obj;

    const char * key() {

        return _obj["key"].as<const char *>();
    }

    const char * value() {

        return _obj["value"].as<const char *>();
    }

};

struct ChangeAvailabilityView {
    JsonObject _obj;

    int32_t connectorId() {

        return _obj["connectorId"].as<int32_t>();
    }

    ChangeAvailabilityType type() {

        return (ChangeAvailabilityType)_obj["type"].as<size_t>();
    }

};

struct BootNotificationResponseView {
    JsonObject _obj;

    BootNotificationResponseStatus status() {

        return (BootNotificationResponseStatus)_obj["status"].as<size_t>();
    }

    time_t currentTime() {

        return _obj["currentTime"].as<time_t>();
    }

    int32_t interval() {

        return _obj["interval"].as<int32_t>();
    }

};

struct AuthorizeResponseIdTagInfoEntriesView {
    JsonObject _obj;

    Opt<time_t> expiryDate() {
        if (!_obj.containsKey("expiryDate"))
                return {false};

        return _obj["expiryDate"].as<time_t>();
    }

    Opt<const char *> parentIdTag() {
        if (!_obj.containsKey("parentIdTag"))
                return {false};

        return _obj["parentIdTag"].as<const char *>();
    }

    AuthorizeResponseIdTagInfoEntriesStatus status() {

        return (AuthorizeResponseIdTagInfoEntriesStatus)_obj["status"].as<size_t>();
    }

};

struct AuthorizeResponseView {
    JsonObject _obj;

    AuthorizeResponseIdTagInfoEntriesView idTagInfo() {

        return AuthorizeResponseIdTagInfoEntriesView{_obj["idTagInfo"].as<JsonObject>()};
    }

};

struct StopTransactionTransactionDataSampledValue {
    const char *value;
    StopTransactionTransactionDataSampledValueContext context = StopTransactionTransactionDataSampledValueContext::NONE;
    StopTransactionTransactionDataSampledValueFormat format = StopTransactionTransactionDataSampledValueFormat::NONE;
    StopTransactionTransactionDataSampledValueMeasurand measurand = StopTransactionTransactionDataSampledValueMeasurand::NONE;
    StopTransactionTransactionDataSampledValuePhase phase = StopTransactionTransactionDataSampledValuePhase::NONE;
    StopTransactionTransactionDataSampledValueLocation location = StopTransactionTransactionDataSampledValueLocation::NONE;
    StopTransactionTransactionDataSampledValueUnit unit = StopTransactionTransactionDataSampledValueUnit::NONE;

    void serializeInto(JsonObject payload);
};

struct MeterValuesMeterValueSampledValue {
    const char *value;
    MeterValuesMeterValueSampledValueContext context = MeterValuesMeterValueSampledValueContext::NONE;
    MeterValuesMeterValueSampledValueFormat format = MeterValuesMeterValueSampledValueFormat::NONE;
    MeterValuesMeterValueSampledValueMeasurand measurand = MeterValuesMeterValueSampledValueMeasurand::NONE;
    MeterValuesMeterValueSampledValuePhase phase = MeterValuesMeterValueSampledValuePhase::NONE;
    MeterValuesMeterValueSampledValueLocation location = MeterValuesMeterValueSampledValueLocation::NONE;
    MeterValuesMeterValueSampledValueUnit unit = MeterValuesMeterValueSampledValueUnit::NONE;

    void serializeInto(JsonObject payload);
};

struct StopTransactionTransactionData {
    time_t timestamp;
    StopTransactionTransactionDataSampledValue *sampledValue; size_t sampledValue_length;

    void serializeInto(JsonObject payload);
};

struct MeterValuesMeterValue {
    time_t timestamp;
    MeterValuesMeterValueSampledValue *sampledValue; size_t sampledValue_length;

    void serializeInto(JsonObject payload);
};

struct GetConfigurationResponseConfigurationKey {
    const char *key;
    bool readonly;
    const char *value = nullptr;

    void serializeInto(JsonObject payload);
};


bool Authorize(DynamicJsonDocument *result,
        const char idTag[21]);

bool BootNotification(DynamicJsonDocument *result,
        const char chargePointVendor[21],
        const char chargePointModel[21],
        const char chargePointSerialNumber[26] = nullptr,
        const char chargeBoxSerialNumber[26] = nullptr,
        const char firmwareVersion[51] = nullptr,
        const char iccid[21] = nullptr,
        const char imsi[21] = nullptr,
        const char meterType[26] = nullptr,
        const char meterSerialNumber[26] = nullptr);

bool ChangeAvailabilityResponse(DynamicJsonDocument *result, const char *call_id,
        ChangeAvailabilityResponseStatus status);

bool ChangeConfigurationResponse(DynamicJsonDocument *result, const char *call_id,
        ChangeConfigurationResponseStatus status);

bool ClearCacheResponse(DynamicJsonDocument *result, const char *call_id,
        ClearCacheResponseStatus status);

bool DataTransfer(DynamicJsonDocument *result,
        const char vendorId[256],
        const char messageId[51] = nullptr,
        const char *data = nullptr);

bool DataTransferResponse(DynamicJsonDocument *result, const char *call_id,
        DataTransferResponseStatus status,
        const char *data = nullptr);

bool GetConfigurationResponse(DynamicJsonDocument *result, const char *call_id,
        GetConfigurationResponseConfigurationKey *configurationKey = nullptr, size_t configurationKey_length = 0,
        const char **unknownKey = nullptr, size_t unknownKey_length = 0);

bool Heartbeat(DynamicJsonDocument *result);

bool MeterValues(DynamicJsonDocument *result,
        int32_t connectorId,
        MeterValuesMeterValue *meterValue, size_t meterValue_length,
        int32_t transactionId = OCPP_INTEGER_NOT_PASSED);

bool RemoteStartTransactionResponse(DynamicJsonDocument *result, const char *call_id,
        RemoteStartTransactionResponseStatus status);

bool RemoteStopTransactionResponse(DynamicJsonDocument *result, const char *call_id,
        RemoteStopTransactionResponseStatus status);

bool ResetResponse(DynamicJsonDocument *result, const char *call_id,
        ResetResponseStatus status);

bool StartTransaction(DynamicJsonDocument *result,
        int32_t connectorId,
        const char idTag[21],
        int32_t meterStart,
        time_t timestamp,
        int32_t reservationId = OCPP_INTEGER_NOT_PASSED);

bool StatusNotification(DynamicJsonDocument *result,
        int32_t connectorId,
        StatusNotificationErrorCode errorCode,
        StatusNotificationStatus status,
        const char info[51] = nullptr,
        time_t timestamp = OCPP_DATETIME_NOT_PASSED,
        const char vendorId[256] = nullptr,
        const char vendorErrorCode[51] = nullptr);

bool StopTransaction(DynamicJsonDocument *result,
        int32_t meterStop,
        time_t timestamp,
        int32_t transactionId,
        const char idTag[21] = nullptr,
        StopTransactionReason reason = StopTransactionReason::NONE,
        StopTransactionTransactionData *transactionData = nullptr, size_t transactionData_length = 0);

bool UnlockConnectorResponse(DynamicJsonDocument *result, const char *call_id,
        UnlockConnectorResponseStatus status);

CallResponse parseAuthorizeResponse(JsonObject obj);

CallResponse parseBootNotificationResponse(JsonObject obj);

CallResponse parseChangeAvailability(JsonObject obj);

CallResponse parseChangeConfiguration(JsonObject obj);

CallResponse parseClearCache(JsonObject obj);

CallResponse parseDataTransfer(JsonObject obj);

CallResponse parseDataTransferResponse(JsonObject obj);

CallResponse parseGetConfiguration(JsonObject obj);

CallResponse parseHeartbeatResponse(JsonObject obj);

CallResponse parseMeterValuesResponse(JsonObject obj);

CallResponse parseRemoteStartTransaction(JsonObject obj);

CallResponse parseRemoteStopTransaction(JsonObject obj);

CallResponse parseReset(JsonObject obj);

CallResponse parseStartTransactionResponse(JsonObject obj);

CallResponse parseStatusNotificationResponse(JsonObject obj);

CallResponse parseStopTransactionResponse(JsonObject obj);

CallResponse parseUnlockConnector(JsonObject obj);

CallResponse callHandler(const char *uid, const char *action_string, JsonObject obj, Ocpp *ocpp);
extern const char *CallActionStrings[];

enum class CallAction {
    AUTHORIZE,
    BOOT_NOTIFICATION,
    CHANGE_AVAILABILITY_RESPONSE,
    CHANGE_CONFIGURATION_RESPONSE,
    CLEAR_CACHE_RESPONSE,
    DATA_TRANSFER,
    DATA_TRANSFER_RESPONSE,
    GET_CONFIGURATION_RESPONSE,
    HEARTBEAT,
    METER_VALUES,
    REMOTE_START_TRANSACTION_RESPONSE,
    REMOTE_STOP_TRANSACTION_RESPONSE,
    RESET_RESPONSE,
    START_TRANSACTION,
    STATUS_NOTIFICATION,
    STOP_TRANSACTION,
    UNLOCK_CONNECTOR_RESPONSE,
    AUTHORIZE_RESPONSE,
    BOOT_NOTIFICATION_RESPONSE,
    CHANGE_AVAILABILITY,
    CHANGE_CONFIGURATION,
    CLEAR_CACHE,
    GET_CONFIGURATION,
    HEARTBEAT_RESPONSE,
    METER_VALUES_RESPONSE,
    REMOTE_START_TRANSACTION,
    REMOTE_STOP_TRANSACTION,
    RESET,
    START_TRANSACTION_RESPONSE,
    STATUS_NOTIFICATION_RESPONSE,
    STOP_TRANSACTION_RESPONSE,
    UNLOCK_CONNECTOR,
    GET_DIAGNOSTICS_RESPONSE,
    DIAGNOSTICS_STATUS_NOTIFICATION,
    FIRMWARE_STATUS_NOTIFICATION,
    UPDATE_FIRMWARE_RESPONSE,
    GET_DIAGNOSTICS,
    DIAGNOSTICS_STATUS_NOTIFICATION_RESPONSE,
    FIRMWARE_STATUS_NOTIFICATION_RESPONSE,
    UPDATE_FIRMWARE,
    GET_LOCAL_LIST_VERSION_RESPONSE,
    SEND_LOCAL_LIST_RESPONSE,
    GET_LOCAL_LIST_VERSION,
    SEND_LOCAL_LIST,
    CANCEL_RESERVATION_RESPONSE,
    RESERVE_NOW_RESPONSE,
    CANCEL_RESERVATION,
    RESERVE_NOW,
    CLEAR_CHARGING_PROFILE_RESPONSE,
    GET_COMPOSITE_SCHEDULE_RESPONSE,
    SET_CHARGING_PROFILE_RESPONSE,
    CLEAR_CHARGING_PROFILE,
    GET_COMPOSITE_SCHEDULE,
    SET_CHARGING_PROFILE,
    TRIGGER_MESSAGE_RESPONSE,
    TRIGGER_MESSAGE
};

CallResponse callResultHandler(CallAction resultTo, JsonObject obj, Ocpp *ocpp);

