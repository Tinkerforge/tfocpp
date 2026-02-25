// THIS FILE IS AUTO-GENERATED. DO NOT EDIT.

#pragma once

#include "Types.h"
#include "TFJson.h"

#include <math.h>

bool iso_string_to_unix_timestamp(const char *iso_string, time_t *t);
void unix_timestamp_to_iso_string(time_t timestamp, TFJsonSerializer &json, const char *key);

class OcppChargePoint;

#define OCPP_INTEGER_NOT_PASSED INT32_MAX
#define OCPP_DECIMAL_NOT_PASSED NAN
#define OCPP_DATETIME_NOT_PASSED 0

extern uint64_t next_call_id;

extern const char * const ChangeAvailabilityResponseStatusStrings[3];
constexpr size_t ChangeAvailabilityResponseStatusStringsMaxLength = 9;

enum class ChangeAvailabilityResponseStatus : uint8_t {
    ACCEPTED,
    REJECTED,
    SCHEDULED,
    NONE_
};

extern const char * const ChangeConfigurationResponseStatusStrings[4];
constexpr size_t ChangeConfigurationResponseStatusStringsMaxLength = 14;

enum class ChangeConfigurationResponseStatus : uint8_t {
    ACCEPTED,
    REJECTED,
    REBOOT_REQUIRED,
    NOT_SUPPORTED,
    NONE_
};

extern const char * const ResponseStatusStrings[2];
constexpr size_t ResponseStatusStringsMaxLength = 8;

enum class ResponseStatus : uint8_t {
    ACCEPTED,
    REJECTED,
    NONE_
};

extern const char * const DataTransferResponseStatusStrings[4];
constexpr size_t DataTransferResponseStatusStringsMaxLength = 16;

enum class DataTransferResponseStatus : uint8_t {
    ACCEPTED,
    REJECTED,
    UNKNOWN_MESSAGE_ID,
    UNKNOWN_VENDOR_ID,
    NONE_
};

extern const char * const StatusNotificationErrorCodeStrings[16];
constexpr size_t StatusNotificationErrorCodeStringsMaxLength = 20;

enum class StatusNotificationErrorCode : uint8_t {
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
    NONE_
};

extern const char * const StatusNotificationStatusStrings[9];
constexpr size_t StatusNotificationStatusStringsMaxLength = 13;

enum class StatusNotificationStatus : uint8_t {
    AVAILABLE,
    PREPARING,
    CHARGING,
    SUSPENDED_EV,
    SUSPENDED_EVSE,
    FINISHING,
    RESERVED,
    UNAVAILABLE,
    FAULTED,
    NONE_
};

extern const char * const StopTransactionReasonStrings[11];
constexpr size_t StopTransactionReasonStringsMaxLength = 14;

enum class StopTransactionReason : uint8_t {
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
    NONE_
};

extern const char * const UnlockConnectorResponseStatusStrings[3];
constexpr size_t UnlockConnectorResponseStatusStringsMaxLength = 12;

enum class UnlockConnectorResponseStatus : uint8_t {
    UNLOCKED,
    UNLOCK_FAILED,
    NOT_SUPPORTED,
    NONE_
};

extern const char * const ResponseIdTagInfoEntriesStatusStrings[5];
constexpr size_t ResponseIdTagInfoEntriesStatusStringsMaxLength = 12;

enum class ResponseIdTagInfoEntriesStatus : uint8_t {
    ACCEPTED,
    BLOCKED,
    EXPIRED,
    INVALID,
    CONCURRENT_TX
};

extern const char * const BootNotificationResponseStatusStrings[3];
constexpr size_t BootNotificationResponseStatusStringsMaxLength = 8;

enum class BootNotificationResponseStatus : uint8_t {
    ACCEPTED,
    PENDING,
    REJECTED
};

extern const char * const ChangeAvailabilityTypeStrings[2];
constexpr size_t ChangeAvailabilityTypeStringsMaxLength = 11;

enum class ChangeAvailabilityType : uint8_t {
    INOPERATIVE,
    OPERATIVE
};

extern const char * const ChargingProfilePurposeStrings[3];
constexpr size_t ChargingProfilePurposeStringsMaxLength = 21;

enum class ChargingProfilePurpose : uint8_t {
    CHARGE_POINT_MAX_PROFILE,
    TX_DEFAULT_PROFILE,
    TX_PROFILE
};

extern const char * const ChargingProfileKindStrings[3];
constexpr size_t ChargingProfileKindStringsMaxLength = 9;

enum class ChargingProfileKind : uint8_t {
    ABSOLUTE,
    RECURRING,
    RELATIVE
};

extern const char * const RecurrencyKindStrings[2];
constexpr size_t RecurrencyKindStringsMaxLength = 6;

enum class RecurrencyKind : uint8_t {
    DAILY,
    WEEKLY
};

extern const char * const ChargingRateUnitStrings[2];
constexpr size_t ChargingRateUnitStringsMaxLength = 1;

enum class ChargingRateUnit : uint8_t {
    A,
    W
};

extern const char * const ResetTypeStrings[2];
constexpr size_t ResetTypeStringsMaxLength = 4;

enum class ResetType : uint8_t {
    HARD,
    SOFT
};

extern const char * const ClearChargingProfileResponseStatusStrings[2];
constexpr size_t ClearChargingProfileResponseStatusStringsMaxLength = 8;

enum class ClearChargingProfileResponseStatus : uint8_t {
    ACCEPTED,
    UNKNOWN,
    NONE_
};

extern const char * const SetChargingProfileResponseStatusStrings[3];
constexpr size_t SetChargingProfileResponseStatusStringsMaxLength = 12;

enum class SetChargingProfileResponseStatus : uint8_t {
    ACCEPTED,
    REJECTED,
    NOT_SUPPORTED,
    NONE_
};

extern const char * const ExtOCMFILStrings[10];
constexpr size_t ExtOCMFILStringsMaxLength = 9;

enum class ExtOCMFIL : uint8_t {
    NONE,
    HEARSAY,
    TRUSTED,
    VERIFIED,
    CERTIFIED,
    SECURE,
    MISMATCH,
    INVALID,
    OUTDATED,
    UNKNOWN,
    NONE_
};

extern const char * const ExtOCMFITStrings[18];
constexpr size_t ExtOCMFITStringsMaxLength = 12;

enum class ExtOCMFIT : uint8_t {
    NONE,
    DENIED,
    UNDEFINED,
    ISO14443,
    ISO15693,
    EMAID,
    EVCCID,
    EVCOID,
    ISO7812,
    CARD_TXN_NR,
    CENTRAL,
    CENTRAL_1,
    CENTRAL_2,
    LOCAL,
    LOCAL_1,
    LOCAL_2,
    PHONE_NUMBER,
    KEY_CODE,
    NONE_
};

extern const char * const ExtOCMFCTStrings[2];
constexpr size_t ExtOCMFCTStringsMaxLength = 6;

enum class ExtOCMFCT : uint8_t {
    EVSEID,
    CBIDC,
    NONE_
};

extern const char * const ExtOCMFWTF_signature_encodingStrings[2];
constexpr size_t ExtOCMFWTF_signature_encodingStringsMaxLength = 6;

enum class ExtOCMFWTF_signature_encoding : uint8_t {
    BASE16,
    BASE64,
    NONE_
};

extern const char * const ExtOCMFIFEntryStrings[17];
constexpr size_t ExtOCMFIFEntryStringsMaxLength = 14;

enum class ExtOCMFIFEntry : uint8_t {
    RFID_NONE,
    RFID_PLAIN,
    RFID_RELATED,
    RFID_PSK,
    OCPP_NONE,
    OCPP_RS,
    OCPP_AUTH,
    OCPP_RS_TLS,
    OCPP_AUTH_TLS,
    OCPP_CACHE,
    OCPP_WHITELIST,
    OCPP_CERTIFIED,
    ISO15118_NONE,
    ISO15118_PNC,
    PLMN_NONE,
    PLMN_RING,
    PLMN_SMS
};

extern const char * const ExtOCMFLCEntriesLUStrings[2];
constexpr size_t ExtOCMFLCEntriesLUStringsMaxLength = 4;

enum class ExtOCMFLCEntriesLU : uint8_t {
    M_OHM,
    U_OHM
};

extern const char * const ExtOCMFRDEntryEntriesTXStrings[10];
constexpr size_t ExtOCMFRDEntryEntriesTXStringsMaxLength = 1;

enum class ExtOCMFRDEntryEntriesTX : uint8_t {
    B,
    C,
    X,
    E,
    L,
    R,
    A,
    P,
    S,
    T
};

extern const char * const ExtOCMFRDEntryEntriesRUStrings[4];
constexpr size_t ExtOCMFRDEntryEntriesRUStringsMaxLength = 4;

enum class ExtOCMFRDEntryEntriesRU : uint8_t {
    K_WH,
    WH,
    M_OHM,
    U_OHM
};

extern const char * const ExtOCMFRDEntryEntriesRTStrings[2];
constexpr size_t ExtOCMFRDEntryEntriesRTStringsMaxLength = 2;

enum class ExtOCMFRDEntryEntriesRT : uint8_t {
    AC,
    DC
};

extern const char * const ExtOCMFRDEntryEntriesEFStrings[5];
constexpr size_t ExtOCMFRDEntryEntriesEFStringsMaxLength = 2;

enum class ExtOCMFRDEntryEntriesEF : uint8_t {
    EMPTY_STRING,
    E,
    T,
    ET,
    T_E
};

extern const char * const ExtOCMFRDEntryEntriesSTStrings[12];
constexpr size_t ExtOCMFRDEntryEntriesSTStringsMaxLength = 1;

enum class ExtOCMFRDEntryEntriesST : uint8_t {
    N,
    G,
    T,
    D,
    R,
    M,
    X,
    I,
    O,
    S,
    E,
    F
};

extern const char * const GetCompositeScheduleResponseChargingScheduleChargingRateUnitStrings[2];
constexpr size_t GetCompositeScheduleResponseChargingScheduleChargingRateUnitStringsMaxLength = 1;

enum class GetCompositeScheduleResponseChargingScheduleChargingRateUnit : uint8_t {
    A,
    W,
    NONE_
};

extern const char * const ExtSMVSignedMeterValueTypeSigningMethodStrings[8];
constexpr size_t ExtSMVSignedMeterValueTypeSigningMethodStringsMaxLength = 27;

enum class ExtSMVSignedMeterValueTypeSigningMethod : uint8_t {
    EMPTY_STRING,
    ECDSASECP192K1_SHA256,
    ECDSASECP256K1_SHA256,
    ECDSASECP192R1_SHA256,
    ECDSASECP256R1_SHA256,
    ECDSABRAINPOOL256R1_SHA256,
    ECDSASECP384R1_SHA256,
    ECDSABRAINPOOL384R1_SHA256,
    NONE_
};

extern const char * const ExtSMVSignedMeterValueTypeEncodingMethodStrings[2];
constexpr size_t ExtSMVSignedMeterValueTypeEncodingMethodStringsMaxLength = 4;

enum class ExtSMVSignedMeterValueTypeEncodingMethod : uint8_t {
    OCMF,
    EDL,
    NONE_
};

extern const char * const ExtOCMFLCLUStrings[2];
constexpr size_t ExtOCMFLCLUStringsMaxLength = 4;

enum class ExtOCMFLCLU : uint8_t {
    M_OHM,
    U_OHM,
    NONE_
};

extern const char * const ExtOCMFRDTXStrings[10];
constexpr size_t ExtOCMFRDTXStringsMaxLength = 1;

enum class ExtOCMFRDTX : uint8_t {
    B,
    C,
    X,
    E,
    L,
    R,
    A,
    P,
    S,
    T,
    NONE_
};

extern const char * const ExtOCMFRDRUStrings[4];
constexpr size_t ExtOCMFRDRUStringsMaxLength = 4;

enum class ExtOCMFRDRU : uint8_t {
    K_WH,
    WH,
    M_OHM,
    U_OHM,
    NONE_
};

extern const char * const ExtOCMFRDRTStrings[2];
constexpr size_t ExtOCMFRDRTStringsMaxLength = 2;

enum class ExtOCMFRDRT : uint8_t {
    AC,
    DC,
    NONE_
};

extern const char * const ExtOCMFRDEFStrings[5];
constexpr size_t ExtOCMFRDEFStringsMaxLength = 2;

enum class ExtOCMFRDEF : uint8_t {
    EMPTY_STRING,
    E,
    T,
    ET,
    T_E,
    NONE_
};

extern const char * const ExtOCMFRDSTStrings[12];
constexpr size_t ExtOCMFRDSTStringsMaxLength = 1;

enum class ExtOCMFRDST : uint8_t {
    N,
    G,
    T,
    D,
    R,
    M,
    X,
    I,
    O,
    S,
    E,
    F,
    NONE_
};

extern const char * const SampledValueContextStrings[8];
constexpr size_t SampledValueContextStringsMaxLength = 18;

enum class SampledValueContext : uint8_t {
    INTERRUPTION_BEGIN,
    INTERRUPTION_END,
    SAMPLE_CLOCK,
    SAMPLE_PERIODIC,
    TRANSACTION_BEGIN,
    TRANSACTION_END,
    TRIGGER,
    OTHER,
    NONE_
};

extern const char * const SampledValueFormatStrings[2];
constexpr size_t SampledValueFormatStringsMaxLength = 10;

enum class SampledValueFormat : uint8_t {
    RAW,
    SIGNED_DATA,
    NONE_
};

extern const char * const SampledValueMeasurandStrings[22];
constexpr size_t SampledValueMeasurandStringsMaxLength = 31;

enum class SampledValueMeasurand : uint8_t {
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
    NONE_
};

extern const char * const SampledValuePhaseStrings[10];
constexpr size_t SampledValuePhaseStringsMaxLength = 5;

enum class SampledValuePhase : uint8_t {
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
    NONE_
};

extern const char * const SampledValueLocationStrings[5];
constexpr size_t SampledValueLocationStringsMaxLength = 6;

enum class SampledValueLocation : uint8_t {
    CABLE,
    EV,
    INLET,
    OUTLET,
    BODY,
    NONE_
};

extern const char * const SampledValueUnitStrings[17];
constexpr size_t SampledValueUnitStringsMaxLength = 10;

enum class SampledValueUnit : uint8_t {
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
    NONE_
};

extern const char * const CallActionStrings[58];
constexpr size_t CallActionStringsMaxLength = 37;

enum class CallAction : uint8_t {
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
    TRIGGER_MESSAGE,
    EXT_SMV,
    EXT_OCMF
};


struct ICall {
    ICall(CallAction action, uint32_t messageId): action(action), ocppJmessageId(messageId), ocppJcallId(nullptr) {}
    ICall(CallAction action, const char *callId): action(action), ocppJmessageId(0), ocppJcallId(callId) {}
    ICall(const ICall &) = delete;
    ICall& operator=(const ICall &) = delete;
    ICall(ICall&&) = default;
    ICall& operator=(ICall&&) = default;

    virtual ~ICall();

    size_t measureJson() const;
    virtual size_t serializeJson(char *buf, size_t buf_len) const = 0;

    CallAction action;
    uint64_t ocppJmessageId;
    const char *ocppJcallId;
};

struct ExtOCMFRDEntryEntriesView {
    JsonObject _obj;

    const char * TM() {

        return _obj["TM"].as<const char *>();
    }

    Option<ExtOCMFRDEntryEntriesTX> TX() {
        if (!_obj.containsKey("TX"))
                return {};

        return Option<ExtOCMFRDEntryEntriesTX>{(ExtOCMFRDEntryEntriesTX)_obj["TX"].as<size_t>()};
    }

    float RV() {

        return _obj["RV"].as<float>();
    }

    Option<const char *> RI() {
        if (!_obj.containsKey("RI"))
                return {};

        return _obj["RI"].as<const char *>();
    }

    ExtOCMFRDEntryEntriesRU RU() {

        return (ExtOCMFRDEntryEntriesRU)_obj["RU"].as<size_t>();
    }

    Option<ExtOCMFRDEntryEntriesRT> RT() {
        if (!_obj.containsKey("RT"))
                return {};

        return Option<ExtOCMFRDEntryEntriesRT>{(ExtOCMFRDEntryEntriesRT)_obj["RT"].as<size_t>()};
    }

    Option<float> CL() {
        if (!_obj.containsKey("CL"))
                return {};

        return _obj["CL"].as<float>();
    }

    Option<ExtOCMFRDEntryEntriesEF> EF() {
        if (!_obj.containsKey("EF"))
                return {};

        return Option<ExtOCMFRDEntryEntriesEF>{(ExtOCMFRDEntryEntriesEF)_obj["EF"].as<size_t>()};
    }

    ExtOCMFRDEntryEntriesST ST() {

        return (ExtOCMFRDEntryEntriesST)_obj["ST"].as<size_t>();
    }

};

struct ExtOCMFLCEntriesView {
    JsonObject _obj;

    Option<const char *> LN() {
        if (!_obj.containsKey("LN"))
                return {};

        return _obj["LN"].as<const char *>();
    }

    Option<int32_t> LI() {
        if (!_obj.containsKey("LI"))
                return {};

        return _obj["LI"].as<int32_t>();
    }

    float LR() {

        return _obj["LR"].as<float>();
    }

    ExtOCMFLCEntriesLU LU() {

        return (ExtOCMFLCEntriesLU)_obj["LU"].as<size_t>();
    }

};

struct ExtOCMFView {
    JsonObject _obj;

    Option<const char *> FV() {
        if (!_obj.containsKey("FV"))
                return {};

        return _obj["FV"].as<const char *>();
    }

    Option<const char *> GI() {
        if (!_obj.containsKey("GI"))
                return {};

        return _obj["GI"].as<const char *>();
    }

    Option<const char *> GS() {
        if (!_obj.containsKey("GS"))
                return {};

        return _obj["GS"].as<const char *>();
    }

    Option<const char *> GV() {
        if (!_obj.containsKey("GV"))
                return {};

        return _obj["GV"].as<const char *>();
    }

    const char * PG() {

        return _obj["PG"].as<const char *>();
    }

    Option<const char *> MV() {
        if (!_obj.containsKey("MV"))
                return {};

        return _obj["MV"].as<const char *>();
    }

    Option<const char *> MM() {
        if (!_obj.containsKey("MM"))
                return {};

        return _obj["MM"].as<const char *>();
    }

    const char * MS() {

        return _obj["MS"].as<const char *>();
    }

    Option<const char *> MF() {
        if (!_obj.containsKey("MF"))
                return {};

        return _obj["MF"].as<const char *>();
    }

    bool IS() {

        return _obj["IS"].as<bool>();
    }

    Option<ExtOCMFIL> IL() {
        if (!_obj.containsKey("IL"))
                return {};

        return Option<ExtOCMFIL>{(ExtOCMFIL)_obj["IL"].as<size_t>()};
    }

    size_t IF_count() {
        if (!_obj.containsKey("IF"))
                return {};

        return _obj["IF"].size();
    }

    Option<ExtOCMFIFEntry> IF(size_t i) {
        if (!_obj.containsKey("IF"))
                return {};

        return (ExtOCMFIFEntry)_obj["IF"][i].as<size_t>();
    }

    ExtOCMFIT IT() {

        return (ExtOCMFIT)_obj["IT"].as<size_t>();
    }

    Option<const char *> ID() {
        if (!_obj.containsKey("ID"))
                return {};

        return _obj["ID"].as<const char *>();
    }

    Option<const char *> TT() {
        if (!_obj.containsKey("TT"))
                return {};

        return _obj["TT"].as<const char *>();
    }

    Option<const char *> CF() {
        if (!_obj.containsKey("CF"))
                return {};

        return _obj["CF"].as<const char *>();
    }

    Option<ExtOCMFLCEntriesView> LC() {
        if (!_obj.containsKey("LC"))
                return {};

        return Option<ExtOCMFLCEntriesView>{ExtOCMFLCEntriesView{_obj["LC"].as<JsonObject>()}};
    }

    Option<ExtOCMFCT> CT() {
        if (!_obj.containsKey("CT"))
                return {};

        return Option<ExtOCMFCT>{(ExtOCMFCT)_obj["CT"].as<size_t>()};
    }

    Option<const char *> CI() {
        if (!_obj.containsKey("CI"))
                return {};

        return _obj["CI"].as<const char *>();
    }

    size_t RD_count() {
        return _obj["RD"].size();
    }

    ExtOCMFRDEntryEntriesView RD(size_t i) {
        return ExtOCMFRDEntryEntriesView{_obj["RD"][i]};
    }

    Option<int32_t> WTF_connector_id() {
        if (!_obj.containsKey("WTF_connector_id"))
                return {};

        return _obj["WTF_connector_id"].as<int32_t>();
    }

    Option<time_t> WTF_unix_time() {
        if (!_obj.containsKey("WTF_unix_time"))
                return {};

        return _obj["WTF_unix_time"].as<time_t>();
    }

    Option<ExtOCMFWTF_signature_encoding> WTF_signature_encoding() {
        if (!_obj.containsKey("WTF_signature_encoding"))
                return {};

        return Option<ExtOCMFWTF_signature_encoding>{(ExtOCMFWTF_signature_encoding)_obj["WTF_signature_encoding"].as<size_t>()};
    }

};

struct SetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesChargingSchedulePeriodEntryEntriesView {
    JsonObject _obj;

    int32_t startPeriod() {

        return _obj["startPeriod"].as<int32_t>();
    }

    float limit() {

        return _obj["limit"].as<float>();
    }

    Option<int32_t> numberPhases() {
        if (!_obj.containsKey("numberPhases"))
                return {};

        return _obj["numberPhases"].as<int32_t>();
    }

};

struct SetChargingProfileCsChargingProfilesEntriesChargingScheduleEntriesView {
    JsonObject _obj;

    Option<int32_t> duration() {
        if (!_obj.containsKey("duration"))
                return {};

        return _obj["duration"].as<int32_t>();
    }

    Option<time_t> startSchedule() {
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

    Option<float> minChargingRate() {
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

    Option<int32_t> transactionId() {
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

    Option<RecurrencyKind> recurrencyKind() {
        if (!_obj.containsKey("recurrencyKind"))
                return {};

        return Option<RecurrencyKind>{(RecurrencyKind)_obj["recurrencyKind"].as<size_t>()};
    }

    Option<time_t> validFrom() {
        if (!_obj.containsKey("validFrom"))
                return {};

        return _obj["validFrom"].as<time_t>();
    }

    Option<time_t> validTo() {
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

    Option<ChargingRateUnit> chargingRateUnit() {
        if (!_obj.containsKey("chargingRateUnit"))
                return {};

        return Option<ChargingRateUnit>{(ChargingRateUnit)_obj["chargingRateUnit"].as<size_t>()};
    }

};

struct ClearChargingProfileView {
    JsonObject _obj;

    Option<int32_t> id() {
        if (!_obj.containsKey("id"))
                return {};

        return _obj["id"].as<int32_t>();
    }

    Option<int32_t> connectorId() {
        if (!_obj.containsKey("connectorId"))
                return {};

        return _obj["connectorId"].as<int32_t>();
    }

    Option<ChargingProfilePurpose> chargingProfilePurpose() {
        if (!_obj.containsKey("chargingProfilePurpose"))
                return {};

        return Option<ChargingProfilePurpose>{(ChargingProfilePurpose)_obj["chargingProfilePurpose"].as<size_t>()};
    }

    Option<int32_t> stackLevel() {
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

    Option<time_t> expiryDate() {
        if (!_obj.containsKey("expiryDate"))
                return {};

        return _obj["expiryDate"].as<time_t>();
    }

    Option<const char *> parentIdTag() {
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

    Option<StopTransactionResponseIdTagInfoEntriesView> idTagInfo() {
        if (!_obj.containsKey("idTagInfo"))
                return {};

        return Option<StopTransactionResponseIdTagInfoEntriesView>{StopTransactionResponseIdTagInfoEntriesView{_obj["idTagInfo"].as<JsonObject>()}};
    }

};

struct StatusNotificationResponseView {
    JsonObject _obj;

};

struct StartTransactionResponseIdTagInfoEntriesView {
    JsonObject _obj;

    Option<time_t> expiryDate() {
        if (!_obj.containsKey("expiryDate"))
                return {};

        return _obj["expiryDate"].as<time_t>();
    }

    Option<const char *> parentIdTag() {
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

    Option<int32_t> numberPhases() {
        if (!_obj.containsKey("numberPhases"))
                return {};

        return _obj["numberPhases"].as<int32_t>();
    }

};

struct RemoteStartTransactionChargingProfileEntriesChargingScheduleEntriesView {
    JsonObject _obj;

    Option<int32_t> duration() {
        if (!_obj.containsKey("duration"))
                return {};

        return _obj["duration"].as<int32_t>();
    }

    Option<time_t> startSchedule() {
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

    Option<float> minChargingRate() {
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

    Option<int32_t> transactionId() {
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

    Option<RecurrencyKind> recurrencyKind() {
        if (!_obj.containsKey("recurrencyKind"))
                return {};

        return Option<RecurrencyKind>{(RecurrencyKind)_obj["recurrencyKind"].as<size_t>()};
    }

    Option<time_t> validFrom() {
        if (!_obj.containsKey("validFrom"))
                return {};

        return _obj["validFrom"].as<time_t>();
    }

    Option<time_t> validTo() {
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

    Option<int32_t> connectorId() {
        if (!_obj.containsKey("connectorId"))
                return {};

        return _obj["connectorId"].as<int32_t>();
    }

    const char * idTag() {

        return _obj["idTag"].as<const char *>();
    }

    Option<RemoteStartTransactionChargingProfileEntriesView> chargingProfile() {
        if (!_obj.containsKey("chargingProfile"))
                return {};

        return Option<RemoteStartTransactionChargingProfileEntriesView>{RemoteStartTransactionChargingProfileEntriesView{_obj["chargingProfile"].as<JsonObject>()}};
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

    Option<const char *> key(size_t i) {
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

    Option<const char *> data() {
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

    Option<const char *> messageId() {
        if (!_obj.containsKey("messageId"))
                return {};

        return _obj["messageId"].as<const char *>();
    }

    Option<const char *> data() {
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

    Option<time_t> expiryDate() {
        if (!_obj.containsKey("expiryDate"))
                return {};

        return _obj["expiryDate"].as<time_t>();
    }

    Option<const char *> parentIdTag() {
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
    SampledValueContext context = SampledValueContext::NONE_;
    SampledValueFormat format = SampledValueFormat::NONE_;
    SampledValueMeasurand measurand = SampledValueMeasurand::NONE_;
    SampledValuePhase phase = SampledValuePhase::NONE_;
    SampledValueLocation location = SampledValueLocation::NONE_;
    SampledValueUnit unit = SampledValueUnit::NONE_;

    void serializeInto(TFJsonSerializer &json);
};

struct ExtOCMFRD {
    const char *TM;
    ExtOCMFRDTX TX = ExtOCMFRDTX::NONE_;
    float RV;
    const char *RI = nullptr;
    ExtOCMFRDRU RU;
    ExtOCMFRDRT RT = ExtOCMFRDRT::NONE_;
    float CL = OCPP_DECIMAL_NOT_PASSED;
    ExtOCMFRDEF EF = ExtOCMFRDEF::NONE_;
    ExtOCMFRDST ST;

    void serializeInto(TFJsonSerializer &json);
};

struct ExtOCMFLC {
    const char *LN = nullptr;
    int32_t LI = OCPP_INTEGER_NOT_PASSED;
    float LR;
    ExtOCMFLCLU LU;

    void serializeInto(TFJsonSerializer &json);
};

struct ExtSMVSignedMeterValueType {
    const char *signedMeterData;
    ExtSMVSignedMeterValueTypeSigningMethod signingMethod;
    ExtSMVSignedMeterValueTypeEncodingMethod encodingMethod;
    const char *publicKey;

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
    Authorize(const Authorize&) = delete;
    Authorize &operator=(const Authorize&) = delete;
    Authorize(Authorize&&) = default;
    Authorize& operator=(Authorize&&) = default;

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
    BootNotification(const BootNotification&) = delete;
    BootNotification &operator=(const BootNotification&) = delete;
    BootNotification(BootNotification&&) = default;
    BootNotification& operator=(BootNotification&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ChangeAvailabilityResponse final : public ICall {
    ChangeAvailabilityResponseStatus status;

    ChangeAvailabilityResponse(const char *call_id,
        ChangeAvailabilityResponseStatus status);
    ChangeAvailabilityResponse(const ChangeAvailabilityResponse&) = delete;
    ChangeAvailabilityResponse &operator=(const ChangeAvailabilityResponse&) = delete;
    ChangeAvailabilityResponse(ChangeAvailabilityResponse&&) = default;
    ChangeAvailabilityResponse& operator=(ChangeAvailabilityResponse&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ChangeConfigurationResponse final : public ICall {
    ChangeConfigurationResponseStatus status;

    ChangeConfigurationResponse(const char *call_id,
        ChangeConfigurationResponseStatus status);
    ChangeConfigurationResponse(const ChangeConfigurationResponse&) = delete;
    ChangeConfigurationResponse &operator=(const ChangeConfigurationResponse&) = delete;
    ChangeConfigurationResponse(ChangeConfigurationResponse&&) = default;
    ChangeConfigurationResponse& operator=(ChangeConfigurationResponse&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ClearCacheResponse final : public ICall {
    ResponseStatus status;

    ClearCacheResponse(const char *call_id,
        ResponseStatus status);
    ClearCacheResponse(const ClearCacheResponse&) = delete;
    ClearCacheResponse &operator=(const ClearCacheResponse&) = delete;
    ClearCacheResponse(ClearCacheResponse&&) = default;
    ClearCacheResponse& operator=(ClearCacheResponse&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct DataTransfer final : public ICall {
    const char *vendorId;
    const char *messageId;
    const char *data;

    DataTransfer(const char vendorId[256],
        const char messageId[51] = nullptr,
        const char *data = nullptr);
    DataTransfer(const DataTransfer&) = delete;
    DataTransfer &operator=(const DataTransfer&) = delete;
    DataTransfer(DataTransfer&&) = default;
    DataTransfer& operator=(DataTransfer&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct DataTransferResponse final : public ICall {
    DataTransferResponseStatus status;
    const char *data;

    DataTransferResponse(const char *call_id,
        DataTransferResponseStatus status,
        const char *data = nullptr);
    DataTransferResponse(const DataTransferResponse&) = delete;
    DataTransferResponse &operator=(const DataTransferResponse&) = delete;
    DataTransferResponse(DataTransferResponse&&) = default;
    DataTransferResponse& operator=(DataTransferResponse&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct GetConfigurationResponse final : public ICall {
    GetConfigurationResponseConfigurationKey *configurationKey; size_t configurationKey_length;
    const char **unknownKey; size_t unknownKey_length;

    GetConfigurationResponse(const char *call_id,
        GetConfigurationResponseConfigurationKey *configurationKey = nullptr, size_t configurationKey_length = 0,
        const char **unknownKey = nullptr, size_t unknownKey_length = 0);
    GetConfigurationResponse(const GetConfigurationResponse&) = delete;
    GetConfigurationResponse &operator=(const GetConfigurationResponse&) = delete;
    GetConfigurationResponse(GetConfigurationResponse&&) = default;
    GetConfigurationResponse& operator=(GetConfigurationResponse&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct Heartbeat final : public ICall {

    Heartbeat();
    Heartbeat(const Heartbeat&) = delete;
    Heartbeat &operator=(const Heartbeat&) = delete;
    Heartbeat(Heartbeat&&) = default;
    Heartbeat& operator=(Heartbeat&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct MeterValues final : public ICall {
    int32_t connectorId;
    int32_t transactionId;
    MeterValue *meterValue; size_t meterValue_length;

    MeterValues(int32_t connectorId,
        MeterValue *meterValue, size_t meterValue_length,
        int32_t transactionId = OCPP_INTEGER_NOT_PASSED);
    MeterValues(const MeterValues&) = delete;
    MeterValues &operator=(const MeterValues&) = delete;
    MeterValues(MeterValues&&) = default;
    MeterValues& operator=(MeterValues&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct RemoteStartTransactionResponse final : public ICall {
    ResponseStatus status;

    RemoteStartTransactionResponse(const char *call_id,
        ResponseStatus status);
    RemoteStartTransactionResponse(const RemoteStartTransactionResponse&) = delete;
    RemoteStartTransactionResponse &operator=(const RemoteStartTransactionResponse&) = delete;
    RemoteStartTransactionResponse(RemoteStartTransactionResponse&&) = default;
    RemoteStartTransactionResponse& operator=(RemoteStartTransactionResponse&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct RemoteStopTransactionResponse final : public ICall {
    ResponseStatus status;

    RemoteStopTransactionResponse(const char *call_id,
        ResponseStatus status);
    RemoteStopTransactionResponse(const RemoteStopTransactionResponse&) = delete;
    RemoteStopTransactionResponse &operator=(const RemoteStopTransactionResponse&) = delete;
    RemoteStopTransactionResponse(RemoteStopTransactionResponse&&) = default;
    RemoteStopTransactionResponse& operator=(RemoteStopTransactionResponse&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ResetResponse final : public ICall {
    ResponseStatus status;

    ResetResponse(const char *call_id,
        ResponseStatus status);
    ResetResponse(const ResetResponse&) = delete;
    ResetResponse &operator=(const ResetResponse&) = delete;
    ResetResponse(ResetResponse&&) = default;
    ResetResponse& operator=(ResetResponse&&) = default;

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
    StartTransaction(const StartTransaction&) = delete;
    StartTransaction &operator=(const StartTransaction&) = delete;
    StartTransaction(StartTransaction&&) = default;
    StartTransaction& operator=(StartTransaction&&) = default;

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
    StatusNotification(const StatusNotification&) = delete;
    StatusNotification &operator=(const StatusNotification&) = delete;
    StatusNotification(StatusNotification&&) = default;
    StatusNotification& operator=(StatusNotification&&) = default;

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
        StopTransactionReason reason = StopTransactionReason::NONE_,
        MeterValue *transactionData = nullptr, size_t transactionData_length = 0);
    StopTransaction(const StopTransaction&) = delete;
    StopTransaction &operator=(const StopTransaction&) = delete;
    StopTransaction(StopTransaction&&) = default;
    StopTransaction& operator=(StopTransaction&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct UnlockConnectorResponse final : public ICall {
    UnlockConnectorResponseStatus status;

    UnlockConnectorResponse(const char *call_id,
        UnlockConnectorResponseStatus status);
    UnlockConnectorResponse(const UnlockConnectorResponse&) = delete;
    UnlockConnectorResponse &operator=(const UnlockConnectorResponse&) = delete;
    UnlockConnectorResponse(UnlockConnectorResponse&&) = default;
    UnlockConnectorResponse& operator=(UnlockConnectorResponse&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ClearChargingProfileResponse final : public ICall {
    ClearChargingProfileResponseStatus status;

    ClearChargingProfileResponse(const char *call_id,
        ClearChargingProfileResponseStatus status);
    ClearChargingProfileResponse(const ClearChargingProfileResponse&) = delete;
    ClearChargingProfileResponse &operator=(const ClearChargingProfileResponse&) = delete;
    ClearChargingProfileResponse(ClearChargingProfileResponse&&) = default;
    ClearChargingProfileResponse& operator=(ClearChargingProfileResponse&&) = default;

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
    GetCompositeScheduleResponse(const GetCompositeScheduleResponse&) = delete;
    GetCompositeScheduleResponse &operator=(const GetCompositeScheduleResponse&) = delete;
    GetCompositeScheduleResponse(GetCompositeScheduleResponse&&) = default;
    GetCompositeScheduleResponse& operator=(GetCompositeScheduleResponse&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct SetChargingProfileResponse final : public ICall {
    SetChargingProfileResponseStatus status;

    SetChargingProfileResponse(const char *call_id,
        SetChargingProfileResponseStatus status);
    SetChargingProfileResponse(const SetChargingProfileResponse&) = delete;
    SetChargingProfileResponse &operator=(const SetChargingProfileResponse&) = delete;
    SetChargingProfileResponse(SetChargingProfileResponse&&) = default;
    SetChargingProfileResponse& operator=(SetChargingProfileResponse&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ExtSMV final : public ICall {
    ExtSMVSignedMeterValueType *signedMeterValueType;

    ExtSMV(const char *call_id,
        ExtSMVSignedMeterValueType *signedMeterValueType = nullptr);
    ExtSMV(const ExtSMV&) = delete;
    ExtSMV &operator=(const ExtSMV&) = delete;
    ExtSMV(ExtSMV&&) = default;
    ExtSMV& operator=(ExtSMV&&) = default;

    size_t serializeJson(char *buf, size_t buf_len) const override;
};

struct ExtOCMF final : public ICall {
    const char *FV;
    const char *GI;
    const char *GS;
    const char *GV;
    const char *PG;
    const char *MV;
    const char *MM;
    const char *MS;
    const char *MF;
    bool IS;
    ExtOCMFIL IL;
    ExtOCMFIFEntry *IF; size_t IF_length;
    ExtOCMFIT IT;
    const char *ID;
    const char *TT;
    const char *CF;
    ExtOCMFLC *LC;
    ExtOCMFCT CT;
    const char *CI;
    ExtOCMFRD *RD; size_t RD_length;
    int32_t WTF_connector_id;
    time_t WTF_unix_time;
    ExtOCMFWTF_signature_encoding WTF_signature_encoding;

    ExtOCMF(const char *PG,
        const char *MS,
        bool IS,
        ExtOCMFIT IT,
        ExtOCMFRD *RD, size_t RD_length,
        const char *FV = nullptr,
        const char GI[42] = nullptr,
        const char GS[26] = nullptr,
        const char GV[51] = nullptr,
        const char *MV = nullptr,
        const char *MM = nullptr,
        const char *MF = nullptr,
        ExtOCMFIL IL = ExtOCMFIL::NONE_,
        ExtOCMFIFEntry *IF = nullptr, size_t IF_length = 0,
        const char *ID = nullptr,
        const char TT[251] = nullptr,
        const char CF[26] = nullptr,
        ExtOCMFLC *LC = nullptr,
        ExtOCMFCT CT = ExtOCMFCT::NONE_,
        const char CI[21] = nullptr,
        int32_t WTF_connector_id = OCPP_INTEGER_NOT_PASSED,
        time_t WTF_unix_time = OCPP_DATETIME_NOT_PASSED,
        ExtOCMFWTF_signature_encoding WTF_signature_encoding = ExtOCMFWTF_signature_encoding::NONE_);
    ExtOCMF(const ExtOCMF&) = delete;
    ExtOCMF &operator=(const ExtOCMF&) = delete;
    ExtOCMF(ExtOCMF&&) = default;
    ExtOCMF& operator=(ExtOCMF&&) = default;

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

CallResponse parseExtOCMF(JsonObject obj);

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
        if (view.parentIdTag().is_some())
            strncpy(parentTagId, view.parentIdTag().unwrap(), ARRAY_SIZE(parentTagId) - 1);

        expiryDate = view.expiryDate().is_some() ? view.expiryDate().unwrap() : 0;
        status = view.status();
    }

    void updateFromIdTagInfo(StartTransactionResponseIdTagInfoEntriesView view) {
        memset(parentTagId, 0, ARRAY_SIZE(parentTagId));
        if (view.parentIdTag().is_some())
            strncpy(parentTagId, view.parentIdTag().unwrap(), ARRAY_SIZE(parentTagId) - 1);

        expiryDate = view.expiryDate().is_some() ? view.expiryDate().unwrap() : 0;
        status = view.status();
    }

    void updateFromIdTagInfo(AuthorizeResponseIdTagInfoEntriesView view) {
        memset(parentTagId, 0, ARRAY_SIZE(parentTagId));
        if (view.parentIdTag().is_some())
            strncpy(parentTagId, view.parentIdTag().unwrap(), ARRAY_SIZE(parentTagId) - 1);

        expiryDate = view.expiryDate().is_some() ? view.expiryDate().unwrap() : 0;
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
