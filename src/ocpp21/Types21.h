#pragma once

#include <stdint.h>
#include <stddef.h>

#include <utility>

#include <ArduinoJson.h>
#include <TFTools/Option.h>

// BasicAuthCredentials and the core platform API
#include <common/Platform.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef OCPP_ISO_8601_MAX_LEN
#define OCPP_ISO_8601_MAX_LEN 36 // 2022-05-19T09:33:53.123456789+02:00 = 35 + 1 for the null terminator.
#endif

namespace Ocpp21 {

extern const char * const CallErrorCodeStrings[];

// RPC framework error codes per OCPP 2.1 part 4, table 9.
// OK and NONE are internal and must stay at the end,
// (size_t)CallErrorCode::OK is used as the string array length.
enum class CallErrorCode {
    NotImplemented = 0,
    NotSupported,
    InternalError,
    ProtocolError,
    SecurityError,
    FormatViolation,
    PropertyConstraintViolation,
    OccurrenceConstraintViolation,
    TypeConstraintViolation,
    GenericError,
    MessageTypeNotSupported,
    RpcFrameworkError,
    OK,
    NONE
};

struct CallResponse {
    CallErrorCode result;
    const char *error_description;
};

// OCPP 2.1 part 4, section 4.1.2
enum class OcppRpcMessageType {
    CALL = 2,
    CALLRESULT = 3,
    CALLERROR = 4,
    CALLRESULTERROR = 5,
    SEND = 6
};

} // namespace Ocpp21
