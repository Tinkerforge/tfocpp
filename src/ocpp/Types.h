#pragma once
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <ArduinoJson.h>
#include <TFTools/Option.h>

#define OCPP_INTEGER_NOT_PASSED INT32_MAX
#define OCPP_DATETIME_NOT_PASSED 0

extern const char * const CallErrorCodeStrings[];
extern const char * const CallErrorCodeStringAliases[];
extern const size_t CallErrorCodeStringAliasIndices[];
extern const size_t CallErrorCodeStringAliasLength;

enum class CallErrorCode {
    NotImplemented = 0, // handled when parsing the outer call
    NotSupported, // handled when parsing the outer call
    InternalError,
    ProtocolError, // For example incomplete payload
    SecurityError,
    FormationViolation, // unknown member, syntax error
    PropertyConstraintViolation, // constraints such as connectorId >= 0 -> seems to be incomplete in schema
    OccurenceConstraintViolation, // member is there, but cardinality is wrong
    TypeConstraintViolation, // correct number of members, but wrong type
    GenericError,
    OK,
    NONE
};

struct CallResponse {
    CallErrorCode result;
    const char *error_description;
    //JsonObject details;
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

enum class OcppRpcMessageType {
    CALL = 2,
    CALLRESULT = 3,
    CALLERROR = 4
};

#define OCPP_CALL_JSON_SIZE(action) (JSON_ARRAY_SIZE(4) + 37 /*message id+\0*/ + strlen(action))
#define OCPP_CALLRESULT_JSON_SIZE(action) (JSON_ARRAY_SIZE(3) + 37 /*message id+\0*/)

#define OCPP_ISO_8601_MAX_LEN 36 // 2022-05-19T09:33:53.123456789+02:00 = 35 + 1 for the null terminator.

enum StopReason {
    //DeAuthorized, // handled by OCPP
    EmergencyStop, // if EVSE emergency stop button is pressed
    //EVDisconnected, detected via platform_get_evse_state
    //HardReset, // handled by OCPP
    Local, // "normal" EVSE stop button
    Other,
    PowerLoss, // "Complete loss of power." maybe use this if contactor check fails (before contactor)
    Reboot, // "A locally initiated reset/reboot occurred. (for instance watchdog kicked in)"
    Remote, // "Stopped remotely on request of the user." maybe use this when stopping over the web interface?
    // SoftReset, // handled by OCPP
    // UnlockCommand, // handled by OCPP
};

struct BasicAuthCredentials {
    const char *user;
    uint8_t *pass;
    size_t pass_length;
};

enum class IdentificationLevel : uint8_t {
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
};

enum class RFIDIdentificationFlag : uint8_t {
    NONE,
    PLAIN,
    RELATED,
    PSK,
    _max = PSK
};

enum class OCPPIdentificationFlag : uint8_t {
    NONE = (size_t)RFIDIdentificationFlag::_max + 1,
    RS,
    AUTH,
    RS_TLS,
    AUTH_TLS,
    CACHE,
    WHITELIST,
    CERTIFIED,
    _max = CERTIFIED
};

enum class ISO15118IdentificationFlag : uint8_t {
    NONE = (size_t)OCPPIdentificationFlag::_max + 1,
    PNC,
    _max = PNC
};

enum class PLMNIdentificationFlag : uint8_t {
    NONE = (size_t)ISO15118IdentificationFlag::_max + 1,
    RING,
    SMS
};

enum class IdentificationType : uint8_t {
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
    KEY_CODE
};

struct LossCompensation {
    const char *naming;
    int32_t identification;
    int32_t cable_resistance;
    enum CableResistanceUnit : uint8_t {
        mOhm,
        uOhm
    } cable_resistance_unit;
};

enum ChargePointIdentificationType : uint8_t {
    EVSEID,
    CBIDC
};

enum SignatureEncoding : uint8_t {
    BASE16,
    BASE64
};
