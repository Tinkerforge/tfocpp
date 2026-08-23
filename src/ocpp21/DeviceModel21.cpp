#include "DeviceModel21.h"

#include <stdio.h>
#include <string.h>

#include <common/Tools.h>

namespace Ocpp21 {

VariableResult DeviceModel21::getVariable(const char *component, const char *variable, char *buf, size_t buf_len)
{
    if (strcmp(component, "OCPPCommCtrlr") == 0) {
        if (strcmp(variable, "HeartbeatInterval") == 0) {
            snprintf(buf, buf_len, "%d", heartbeat_interval_s);
            return VariableResult::Accepted;
        }
        return VariableResult::UnknownVariable;
    }

    return VariableResult::UnknownComponent;
}

VariableResult DeviceModel21::setVariable(const char *component, const char *variable, const char *value)
{
    if (strcmp(component, "OCPPCommCtrlr") == 0) {
        if (strcmp(variable, "HeartbeatInterval") == 0) {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() <= 0)
                return VariableResult::Rejected;
            heartbeat_interval_s = parsed.unwrap();
            return VariableResult::Accepted;
        }
        return VariableResult::UnknownVariable;
    }

    return VariableResult::UnknownComponent;
}

} // namespace Ocpp21
