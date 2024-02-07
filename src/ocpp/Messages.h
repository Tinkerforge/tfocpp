// THIS FILE IS AUTO-GENERATED. DO NOT EDIT.

#pragma once

#include "Types.h"
#include "TFJson.h"

#include <math.h>

class OcppChargePoint;

#define OCPP_INTEGER_NOT_PASSED INT32_MAX
#define OCPP_DECIMAL_NOT_PASSED NAN
#define OCPP_DATETIME_NOT_PASSED 0

extern uint64_t next_call_id;

extern const char * const ChangeAvailabilityResponseStatusStrings[];

enum class ChangeAvailabilityResponseStatus {
    ACCEPTED,
    REJECTED,
    SCHEDULED,
    NONE
};

extern const char * const ChangeConfigurationResponseStatusStrings[];

enum class ChangeConfigurationResponseStatus {
    ACCEPTED,
    REJECTED,
    REBOOT_REQUIRED,
    NOT_SUPPORTED,
    NONE
};

extern const char * const ResponseStatusStrings[];

enum class ResponseStatus {
    ACCEPTED,
    REJECTED,
    NONE
};

extern const char * const DataTransferResponseStatusStrings[];

enum class DataTransferResponseStatus {
    ACCEPTED,
    REJECTED,
    UNKNOWN_MESSAGE_ID,
    UNKNOWN_VENDOR_ID,
    NONE
};

extern const char * const StatusNotificationErrorCodeStrings[];

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

extern const char * const StatusNotificationStatusStrings[];

enum class StatusNotificationStatus {
    AVAILABLE,
    PREPARING,
    CHARGING,
    SUSPENDED_EV,
    SUSPENDED_EVSE,
    FINISHING,
    RESERVED,
    UNAVAILABLE,
    FAULTED,
    NONE
};

extern const char * const StopTransactionReasonStrings[];

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

extern const char * const UnlockConnectorResponseStatusStrings[];

enum class UnlockConnectorResponseStatus {
    UNLOCKED,
    UNLOCK_FAILED,
    NOT_SUPPORTED,
    NONE
};

extern const char * const ResponseIdTagInfoEntriesStatusStrings[];

enum class ResponseIdTagInfoEntriesStatus {
    ACCEPTED,
    BLOCKED,
    EXPIRED,
    INVALID,
    CONCURRENT_TX
};

extern const char * const BootNotificationResponseStatusStrings[];

enum class BootNotificationResponseStatus {
    ACCEPTED,
    PENDING,
    REJECTED
};

extern const char * const ChangeAvailabilityTypeStrings[];

enum class ChangeAvailabilityType {
    INOPERATIVE,
    OPERATIVE
};

extern const char * const ChargingProfilePurposeStrings[];

enum class ChargingProfilePurpose {
    CHARGE_POINT_MAX_PROFILE,
    TX_DEFAULT_PROFILE,
    TX_PROFILE
};

extern const char * const ChargingProfileKindStrings[];

enum class ChargingProfileKind {
    ABSOLUTE,
    RECURRING,
    RELATIVE
};

extern const char * const RecurrencyKindStrings[];

enum class RecurrencyKind {
    DAILY,
    WEEKLY
};

extern const char * const ChargingRateUnitStrings[];

enum class ChargingRateUnit {
    A,
    W
};

extern const char * const ResetTypeStrings[];

enum class ResetType {
    HARD,
    SOFT
};

extern const char * const ClearChargingProfileResponseStatusStrings[];

enum class ClearChargingProfileResponseStatus {
    ACCEPTED,
    UNKNOWN,
    NONE
};

extern const char * const SetChargingProfileResponseStatusStrings[];

enum class SetChargingProfileResponseStatus {
    ACCEPTED,
    REJECTED,
    NOT_SUPPORTED,
    NONE
};

extern const char * const GetCompositeScheduleResponseChargingScheduleChargingRateUnitStrings[];

enum class GetCompositeScheduleResponseChargingScheduleChargingRateUnit {
    A,
    W,
    NONE
};

extern const char * const SampledValueContextStrings[];

enum class SampledValueContext {
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

extern const char * const SampledValueFormatStrings[];

enum class SampledValueFormat {
    RAW,
    SIGNED_DATA,
    NONE
};

extern const char * const SampledValueMeasurandStrings[];

enum class SampledValueMeasurand {
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

extern const char * const SampledValuePhaseStrings[];

enum class SampledValuePhase {
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

extern const char * const SampledValueLocationStrings[];

enum class SampledValueLocation {
    CABLE,
    EV,
    INLET,
    OUTLET,
    BODY,
    NONE
};

extern const char * const SampledValueUnitStrings[];

enum class SampledValueUnit {
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
    CELSIUS,
    FAHRENHEIT,
    PERCENT,
    NONE
};

extern const char * const CallActionStrings[];

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


struct ICall {
    ICall(CallAction action, uint32_t messageId): action(action), ocppJmessageId(messageId), ocppJcallId(nullptr) {}
    ICall(CallAction action, const char *callId): action(action), ocppJmessageId(0), ocppJcallId(callId) {}
    ICall(const ICall &) = delete;

    virtual ~ICall();

    size_t measureJson() const;
    virtual size_t serializeJson(char *buf, size_t buf_len) const = 0;

    CallAction action;
    uint64_t ocppJmessageId;
    const char *ocppJcallId;
};

struct SetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesView {
    JsonObject _obj;

    int32_t startPeriod() {

        return _obj["startPeriod"].as<int32_t>();
    }

    float limit() {

        return _obj["limit"].as<float>();
    }

    Opt<int32_t> numberPhases() {
        if (!_obj.containsKey("numberPhases"))
                return {};

        return _obj["numberPhases"].as<int32_t>();
    }

};

struct SetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesView {
    JsonObject _obj;

    Opt<int32_t> duration() {
        if (!_obj.containsKey("duration"))
                return {};

        return _obj["duration"].as<int32_t>();
    }

    Opt<time_t> startSchedule() {
        if (!_obj.containsKey("startSchedule"))
                return {};

        return _obj["startSchedule"].as<time_t>();
    }

    ChargingRateUnit chargingRateUnit() {

        return (ChargingRateUnit)_obj["chargingRateUnit"].as<size_t>();
    }

    size_t chargingSchedulePeriod_count() {

        return _obj["chargingSchedulePeriod"].size();
    }

    SetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesView chargingSchedulePeriod(size_t i) {

        return SetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesView{_obj["chargingSchedulePeriod"][i]};
    }

    Opt<float> minChargingRate() {
        if (!_obj.containsKey("minChargingRate"))
                return {};

        return _obj["minChargingRate"].as<float>();
    }

};

struct SetChargingProfileCsChargingProfilesEntriesView {
    JsonObject _obj;

    int32_t chargingProfileId() {

        return _obj["chargingProfileId"].as<int32_t>();
    }

    Opt<int32_t> transactionId() {
        if (!_obj.containsKey("transactionId"))
                return {};

        return _obj["transactionId"].as<int32_t>();
    }

    int32_t stackLevel() {

        return _obj["stackLevel"].as<int32_t>();
    }

    ChargingProfilePurpose chargingProfilePurpose() {

        return (ChargingProfilePurpose)_obj["chargingProfilePurpose"].as<size_t>();
    }

    ChargingProfileKind chargingProfileKind() {

        return (ChargingProfileKind)_obj["chargingProfileKind"].as<size_t>();
    }

    Opt<RecurrencyKind> recurrencyKind() {
        if (!_obj.containsKey("recurrencyKind"))
                return {};

        return Opt<RecurrencyKind>{(RecurrencyKind)_obj["recurrencyKind"].as<size_t>()};
    }

    Opt<time_t> validFrom() {
        if (!_obj.containsKey("validFrom"))
                return {};

        return _obj["validFrom"].as<time_t>();
    }

    Opt<time_t> validTo() {
        if (!_obj.containsKey("validTo"))
                return {};

        return _obj["validTo"].as<time_t>();
    }

    SetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesView chargingSchedule() {

        return SetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesView{_obj["chargingSchedule"].as<JsonObject>()};
    }

};

struct SetChargingProfileView {
    JsonObject _obj;

    int32_t connectorId() {

        return _obj["connectorId"].as<int32_t>();
    }

    SetChargingProfileCsChargingProfilesEntriesView csChargingProfiles() {

        return SetChargingProfileCsChargingProfilesEntriesView{_obj["csChargingProfiles"].as<JsonObject>()};
    }

};

struct GetCompositeScheduleView {
    JsonObject _obj;

    int32_t connectorId() {

        return _obj["connectorId"].as<int32_t>();
    }

    int32_t duration() {

        return _obj["duration"].as<int32_t>();
    }

    Opt<ChargingRateUnit> chargingRateUnit() {
        if (!_obj.containsKey("chargingRateUnit"))
                return {};

        return Opt<ChargingRateUnit>{(ChargingRateUnit)_obj["chargingRateUnit"].as<size_t>()};
    }

};

struct ClearChargingProfileView {
    JsonObject _obj;

    Opt<int32_t> id() {
        if (!_obj.containsKey("id"))
                return {};

        return _obj["id"].as<int32_t>();
    }

    Opt<int32_t> connectorId() {
        if (!_obj.containsKey("connectorId"))
                return {};

        return _obj["connectorId"].as<int32_t>();
    }

    Opt<ChargingProfilePurpose> chargingProfilePurpose() {
        if (!_obj.containsKey("chargingProfilePurpose"))
                return {};

        return Opt<ChargingProfilePurpose>{(ChargingProfilePurpose)_obj["chargingProfilePurpose"].as<size_t>()};
    }

    Opt<int32_t> stackLevel() {
        if (!_obj.containsKey("stackLevel"))
                return {};

        return _obj["stackLevel"].as<int32_t>();
    }

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
                return {};

        return _obj["expiryDate"].as<time_t>();
    }

    Opt<const char *> parentIdTag() {
        if (!_obj.containsKey("parentIdTag"))
                return {};

        return _obj["parentIdTag"].as<const char *>();
    }

    ResponseIdTagInfoEntriesStatus status() {

        return (ResponseIdTagInfoEntriesStatus)_obj["status"].as<size_t>();
    }

};

struct StopTransactionResponseView {
    JsonObject _obj;

    Opt<StopTransactionResponseIdTagInfoEntriesView> idTagInfo() {
        if (!_obj.containsKey("idTagInfo"))
                return {};

        return Opt<StopTransactionResponseIdTagInfoEntriesView>{StopTransactionResponseIdTagInfoEntriesView{_obj["idTagInfo"].as<JsonObject>()}};
    }

};

struct StatusNotificationResponseView {
    JsonObject _obj;

};

struct StartTransactionResponseIdTagInfoEntriesView {
    JsonObject _obj;

    Opt<time_t> expiryDate() {
        if (!_obj.containsKey("expiryDate"))
                return {};

        return _obj["expiryDate"].as<time_t>();
    }

    Opt<const char *> parentIdTag() {
        if (!_obj.containsKey("parentIdTag"))
                return {};

        return _obj["parentIdTag"].as<const char *>();
    }

    ResponseIdTagInfoEntriesStatus status() {

        return (ResponseIdTagInfoEntriesStatus)_obj["status"].as<size_t>();
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
                return {};

        return _obj["numberPhases"].as<int32_t>();
    }

};

struct RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesView {
    JsonObject _obj;

    Opt<int32_t> duration() {
        if (!_obj.containsKey("duration"))
                return {};

        return _obj["duration"].as<int32_t>();
    }

    Opt<time_t> startSchedule() {
        if (!_obj.containsKey("startSchedule"))
                return {};

        return _obj["startSchedule"].as<time_t>();
    }

    ChargingRateUnit chargingRateUnit() {

        return (ChargingRateUnit)_obj["chargingRateUnit"].as<size_t>();
    }

    size_t chargingSchedulePeriod_count() {

        return _obj["chargingSchedulePeriod"].size();
    }

    RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesView chargingSchedulePeriod(size_t i) {

        return RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesView{_obj["chargingSchedulePeriod"][i]};
    }

    Opt<float> minChargingRate() {
        if (!_obj.containsKey("minChargingRate"))
                return {};

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
                return {};

        return _obj["transactionId"].as<int32_t>();
    }

    int32_t stackLevel() {

        return _obj["stackLevel"].as<int32_t>();
    }

    ChargingProfilePurpose chargingProfilePurpose() {

        return (ChargingProfilePurpose)_obj["chargingProfilePurpose"].as<size_t>();
    }

    ChargingProfileKind chargingProfileKind() {

        return (ChargingProfileKind)_obj["chargingProfileKind"].as<size_t>();
    }

    Opt<RecurrencyKind> recurrencyKind() {
        if (!_obj.containsKey("recurrencyKind"))
                return {};

        return Opt<RecurrencyKind>{(RecurrencyKind)_obj["recurrencyKind"].as<size_t>()};
    }

    Opt<time_t> validFrom() {
        if (!_obj.containsKey("validFrom"))
                return {};

        return _obj["validFrom"].as<time_t>();
    }

    Opt<time_t> validTo() {
        if (!_obj.containsKey("validTo"))
                return {};

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
                return {};

        return _obj["connectorId"].as<int32_t>();
    }

    const char * idTag() {

        return _obj["idTag"].as<const char *>();
    }

    Opt<RemoteStartTransactionChargingProfileEntriesView> chargingProfile() {
        if (!_obj.containsKey("chargingProfile"))
                return {};

        return Opt<RemoteStartTransactionChargingProfileEntriesView>{RemoteStartTransactionChargingProfileEntriesView{_obj["chargingProfile"].as<JsonObject>()}};
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
                return {};

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
                return {};

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
                return {};

        return _obj["messageId"].as<const char *>();
    }

    Opt<const char *> data() {
        if (!_obj.containsKey("data"))
                return {};

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
                return {};

        return _obj["expiryDate"].as<time_t>();
    }

    Opt<const char *> parentIdTag() {
        if (!_obj.containsKey("parentIdTag"))
                return {};

        return _obj["parentIdTag"].as<const char *>();
    }

    ResponseIdTagInfoEntriesStatus status() {

        return (ResponseIdTagInfoEntriesStatus)_obj["status"].as<size_t>();
    }

};

struct AuthorizeResponseView {
    JsonObject _obj;

    AuthorizeResponseIdTagInfoEntriesView idTagInfo() {

        return AuthorizeResponseIdTagInfoEntriesView{_obj["idTagInfo"].as<JsonObject>()};
    }

};

struct GetCompositeScheduleResponseChargingScheduleChargingSchedulePeriod {
    int32_t startPeriod;
    float limit;
    int32_t numberPhases = OCPP_INTEGER_NOT_PASSED;

    void serializeInto(TFJsonSerializer &json);
};

struct MeterValueSampledValue {
    const char *value;
    SampledValueContext context = SampledValueContext::NONE;
    SampledValueFormat format = SampledValueFormat::NONE;
    SampledValueMeasurand measurand = SampledValueMeasurand::NONE;
    SampledValuePhase phase = SampledValuePhase::NONE;
    SampledValueLocation location = SampledValueLocation::NONE;
    SampledValueUnit unit = SampledValueUnit::NONE;

    void serializeInto(TFJsonSerializer &json);
};

struct GetCompositeScheduleResponseChargingSchedule {
    int32_t duration = OCPP_INTEGER_NOT_PASSED;
    time_t startSchedule = OCPP_DATETIME_NOT_PASSED;
    GetCompositeScheduleResponseChargingScheduleChargingRateUnit chargingRateUnit;
    GetCompositeScheduleResponseChargingScheduleChargingSchedulePeriod *chargingSchedulePeriod; size_t chargingSchedulePeriod_length;
    float minChargingRate = OCPP_DECIMAL_NOT_PASSED;

    void serializeInto(TFJsonSerializer &json);
};

struct MeterValue {
    time_t timestamp;
    MeterValueSampledValue *sampledValue; size_t sampledValue_length;

    void serializeInto(TFJsonSerializer &json);
};

struct GetConfigurationResponseConfigurationKey {
    const char *key;
    bool readonly;
    const char *value = nullptr;

    void serializeInto(TFJsonSerializer &json);
};


CallResponse callHandler(const char *uid, const char *action_string, JsonObject obj, OcppChargePoint *cp);

struct Authorize final : public ICall {
    const char *idTag;

    Authorize(const char idTag[21]);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct BootNotification final : public ICall {
    const char *chargePointVendor;
    const char *chargePointModel;
    const char *chargePointSerialNumber;
    const char *chargeBoxSerialNumber;
    const char *firmwareVersion;
    const char *iccid;
    const char *imsi;
    const char *meterType;
    const char *meterSerialNumber;

    BootNotification(const char chargePointVendor[21],
        const char chargePointModel[21],
        const char chargePointSerialNumber[26] = nullptr,
        const char chargeBoxSerialNumber[26] = nullptr,
        const char firmwareVersion[51] = nullptr,
        const char iccid[21] = nullptr,
        const char imsi[21] = nullptr,
        const char meterType[26] = nullptr,
        const char meterSerialNumber[26] = nullptr);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ChangeAvailabilityResponse final : public ICall {
    ChangeAvailabilityResponseStatus status;

    ChangeAvailabilityResponse(const char *call_id,
        ChangeAvailabilityResponseStatus status);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ChangeConfigurationResponse final : public ICall {
    ChangeConfigurationResponseStatus status;

    ChangeConfigurationResponse(const char *call_id,
        ChangeConfigurationResponseStatus status);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ClearCacheResponse final : public ICall {
    ResponseStatus status;

    ClearCacheResponse(const char *call_id,
        ResponseStatus status);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct DataTransfer final : public ICall {
    const char *vendorId;
    const char *messageId;
    const char *data;

    DataTransfer(const char vendorId[256],
        const char messageId[51] = nullptr,
        const char *data = nullptr);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct DataTransferResponse final : public ICall {
    DataTransferResponseStatus status;
    const char *data;

    DataTransferResponse(const char *call_id,
        DataTransferResponseStatus status,
        const char *data = nullptr);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct GetConfigurationResponse final : public ICall {
    GetConfigurationResponseConfigurationKey *configurationKey; size_t configurationKey_length;
    const char **unknownKey; size_t unknownKey_length;

    GetConfigurationResponse(const char *call_id,
        GetConfigurationResponseConfigurationKey *configurationKey = nullptr, size_t configurationKey_length = 0,
        const char **unknownKey = nullptr, size_t unknownKey_length = 0);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct Heartbeat final : public ICall {

    Heartbeat();

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct MeterValues final : public ICall {
    int32_t connectorId;
    int32_t transactionId;
    MeterValue *meterValue; size_t meterValue_length;

    MeterValues(int32_t connectorId,
        MeterValue *meterValue, size_t meterValue_length,
        int32_t transactionId = OCPP_INTEGER_NOT_PASSED);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct RemoteStartTransactionResponse final : public ICall {
    ResponseStatus status;

    RemoteStartTransactionResponse(const char *call_id,
        ResponseStatus status);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct RemoteStopTransactionResponse final : public ICall {
    ResponseStatus status;

    RemoteStopTransactionResponse(const char *call_id,
        ResponseStatus status);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ResetResponse final : public ICall {
    ResponseStatus status;

    ResetResponse(const char *call_id,
        ResponseStatus status);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct StartTransaction final : public ICall {
    int32_t connectorId;
    const char *idTag;
    int32_t meterStart;
    int32_t reservationId;
    time_t timestamp;

    StartTransaction(int32_t connectorId,
        const char idTag[21],
        int32_t meterStart,
        time_t timestamp,
        int32_t reservationId = OCPP_INTEGER_NOT_PASSED);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct StatusNotification final : public ICall {
    int32_t connectorId;
    StatusNotificationErrorCode errorCode;
    const char *info;
    StatusNotificationStatus status;
    time_t timestamp;
    const char *vendorId;
    const char *vendorErrorCode;

    StatusNotification(int32_t connectorId,
        StatusNotificationErrorCode errorCode,
        StatusNotificationStatus status,
        const char info[51] = nullptr,
        time_t timestamp = OCPP_DATETIME_NOT_PASSED,
        const char vendorId[256] = nullptr,
        const char vendorErrorCode[51] = nullptr);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct StopTransaction final : public ICall {
    const char *idTag;
    int32_t meterStop;
    time_t timestamp;
    int32_t transactionId;
    StopTransactionReason reason;
    MeterValue *transactionData; size_t transactionData_length;

    StopTransaction(int32_t meterStop,
        time_t timestamp,
        int32_t transactionId,
        const char idTag[21] = nullptr,
        StopTransactionReason reason = StopTransactionReason::NONE,
        MeterValue *transactionData = nullptr, size_t transactionData_length = 0);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct UnlockConnectorResponse final : public ICall {
    UnlockConnectorResponseStatus status;

    UnlockConnectorResponse(const char *call_id,
        UnlockConnectorResponseStatus status);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ClearChargingProfileResponse final : public ICall {
    ClearChargingProfileResponseStatus status;

    ClearChargingProfileResponse(const char *call_id,
        ClearChargingProfileResponseStatus status);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct GetCompositeScheduleResponse final : public ICall {
    ResponseStatus status;
    int32_t connectorId;
    time_t scheduleStart;
    GetCompositeScheduleResponseChargingSchedule *chargingSchedule;

    GetCompositeScheduleResponse(const char *call_id,
        ResponseStatus status,
        int32_t connectorId = OCPP_INTEGER_NOT_PASSED,
        time_t scheduleStart = OCPP_DATETIME_NOT_PASSED,
        GetCompositeScheduleResponseChargingSchedule *chargingSchedule = nullptr);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct SetChargingProfileResponse final : public ICall {
    SetChargingProfileResponseStatus status;

    SetChargingProfileResponse(const char *call_id,
        SetChargingProfileResponseStatus status);

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

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

CallResponse parseClearChargingProfile(JsonObject obj);

CallResponse parseGetCompositeSchedule(JsonObject obj);

CallResponse parseSetChargingProfile(JsonObject obj);

CallResponse callResultHandler(int32_t connectorId, CallAction resultTo, JsonObject obj, OcppChargePoint *cp);


struct IdTagInfo {
    char tagId[21] = {0};
    char parentTagId[21] = {0};
    ResponseIdTagInfoEntriesStatus status = ResponseIdTagInfoEntriesStatus::INVALID;
    time_t expiryDate = 0;

    void updateTagId(const char *newTagId) {
        memset(tagId, 0, ARRAY_SIZE(tagId));
        strncpy(tagId, newTagId, ARRAY_SIZE(tagId) - 1);
    }

     void updateFromIdTagInfo(IdTagInfo view) {
        memset(parentTagId, 0, ARRAY_SIZE(parentTagId));
        strncpy(parentTagId, view.parentTagId, ARRAY_SIZE(parentTagId) - 1);

        expiryDate = view.expiryDate;
        status = view.status;
    }

    void updateFromIdTagInfo(StopTransactionResponseIdTagInfoEntriesView view) {
        memset(parentTagId, 0, ARRAY_SIZE(parentTagId));
        if (view.parentIdTag().is_set())
            strncpy(parentTagId, view.parentIdTag().get(), ARRAY_SIZE(parentTagId) - 1);

        expiryDate = view.expiryDate().is_set() ? view.expiryDate().get() : 0;
        status = view.status();
    }

    void updateFromIdTagInfo(StartTransactionResponseIdTagInfoEntriesView view) {
        memset(parentTagId, 0, ARRAY_SIZE(parentTagId));
        if (view.parentIdTag().is_set())
            strncpy(parentTagId, view.parentIdTag().get(), ARRAY_SIZE(parentTagId) - 1);

        expiryDate = view.expiryDate().is_set() ? view.expiryDate().get() : 0;
        status = view.status();
    }

    void updateFromIdTagInfo(AuthorizeResponseIdTagInfoEntriesView view) {
        memset(parentTagId, 0, ARRAY_SIZE(parentTagId));
        if (view.parentIdTag().is_set())
            strncpy(parentTagId, view.parentIdTag().get(), ARRAY_SIZE(parentTagId) - 1);

        expiryDate = view.expiryDate().is_set() ? view.expiryDate().get() : 0;
        status = view.status();
    }

    bool is_same_group(IdTagInfo *other) {
        return (strncmp(this->tagId, other->tagId, ARRAY_SIZE(this->tagId)) == 0)
            || (strncmp(this->parentTagId, other->parentTagId, ARRAY_SIZE(this->parentTagId)) == 0);
    }

    bool is_same_tag(const char *other_id) {
        return (strncmp(this->tagId, other_id, ARRAY_SIZE(this->tagId)) == 0);
    }
};
