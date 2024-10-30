#pragma once
#include "stdint.h"
#include "stddef.h"
#include "time.h"
#include "ArduinoJson.h"

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

extern void platform_abort(const char *);

template<typename T>
struct Option {
public:
    template<typename U = std::is_trivially_copy_constructible<T>, typename std::enable_if<U::value>::type...>
    Option(T t): val(t), have_val(true) {}

    Option() : have_val(false) {}

    T &unwrap() {
        return expect("unwrapped Option without value!");
    }

    const T &unwrap() const {
        return expect("unwrapped Option without value!");
    }

    T &unwrap_or(T &default_value) {
        if (!have_val)
            return default_value;
        return val;
    }

    const T &unwrap_or(const T &default_value) const {
        if (!have_val)
            return default_value;
        return val;
    }

    T &expect(const char *message) {
        if (!have_val)
            platform_abort(message);
        return val;
    }

    const T &expect(const char *message) const {
        if (!have_val)
            platform_abort(message);
        return val;
    }

    T &insert(const T &default_value) {
        val = default_value;
        have_val = true;
        return val;
    }

    bool is_some() const {
        return have_val;
    }

    bool is_none() const {
        return !have_val;
    }

    void clear() {
        have_val = false;
    }

private:
    T val;
    bool have_val;
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
