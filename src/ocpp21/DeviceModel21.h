#pragma once

#include <stdint.h>
#include <stddef.h>

namespace Ocpp21 {

// Minimal device model:
// a flat compile time variable list behind the Component/Variable wire API.
// Attributes are limited to Actual. Extended on demand.

enum class VariableResult : uint8_t {
    Accepted,
    Rejected,
    UnknownComponent,
    UnknownVariable,
    NotSupportedAttributeType,
};

class DeviceModel21 {
public:
    // Values owned by the device model.
    int32_t heartbeat_interval_s = 300;

    // On success writes the value as string into buf.
    VariableResult getVariable(const char *component, const char *variable, char *buf, size_t buf_len);
    VariableResult setVariable(const char *component, const char *variable, const char *value);
};

} // namespace Ocpp21
