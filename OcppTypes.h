#pragma once
#include "stdint.h"
#include "stddef.h"
#include "time.h"
#include "ArduinoJson/ArduinoJson-v6.19.4.h"

#define OCPP_INTEGER_NOT_PASSED INT32_MAX
#define OCPP_DATETIME_NOT_PASSED 0

extern const char *CallErrorCodeStrings[];

enum CallErrorCode {
    NotImplemented = 0, // handled when parsing the outer call
    NotSupported, // handled when parsing the outer call
    InternalError, // ?
    ProtocolError, // parsing error
    SecurityError, // eeh
    FormationViolation, // unknown member, syntax error
    PropertyConstraintViolation, // constraints such as connectorId >= 0 -> seems to be incomplete in schema
    OccurenceConstraintViolation, // member is there, but cardinality is wrong
    TypeConstraintViolation, // correct number of members, but wrong type
    GenericError,
    OK
};

struct CallResponse {
    CallErrorCode result;
    const char *error_description;
    //JsonObject details;
};

template<typename T>
struct Opt {
public:
    Opt(T t): val(t), have_val(true) {}
    Opt(bool have): val(), have_val(have) {}

    T get() {
        return val;
    }

    bool is_set() {
        return have_val;
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

