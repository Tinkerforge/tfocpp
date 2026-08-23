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

    if (strcmp(component, "TxCtrlr") == 0) {
        if (strcmp(variable, "EVConnectionTimeOut") == 0) {
            snprintf(buf, buf_len, "%d", ev_connection_timeout_s);
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "StopTxOnEVSideDisconnect") == 0) {
            snprintf(buf, buf_len, "true");
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "TxStartPoint") == 0 || strcmp(variable, "TxStopPoint") == 0) {
            snprintf(buf, buf_len, "PowerPathClosed");
            return VariableResult::Accepted;
        }
        return VariableResult::UnknownVariable;
    }

    if (strcmp(component, "SampledDataCtrlr") == 0) {
        if (strcmp(variable, "TxUpdatedInterval") == 0) {
            snprintf(buf, buf_len, "%d", tx_updated_interval_s);
            return VariableResult::Accepted;
        }
        return VariableResult::UnknownVariable;
    }

    if (strcmp(component, "AuthCtrlr") == 0) {
        if (strcmp(variable, "AuthorizeRemoteStart") == 0) {
            snprintf(buf, buf_len, "false");
            return VariableResult::Accepted;
        }
        return VariableResult::UnknownVariable;
    }

    if (strcmp(component, "SecurityCtrlr") == 0) {
        if (strcmp(variable, "SecurityProfile") == 0) {
            snprintf(buf, buf_len, "%d", security_profile);
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "Identity") == 0) {
            if (identity == nullptr)
                return VariableResult::Rejected;
            snprintf(buf, buf_len, "%s", identity);
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "OrganizationName") == 0) {
            snprintf(buf, buf_len, "%s", organization_name);
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "CertificateEntries") == 0) {
            // No certificate store yet.
            snprintf(buf, buf_len, "0");
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "BasicAuthPassword") == 0) {
            // WriteOnly, reads are rejected (B06.FR.09).
            return VariableResult::Rejected;
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

    if (strcmp(component, "TxCtrlr") == 0) {
        if (strcmp(variable, "EVConnectionTimeOut") == 0) {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() < 0)
                return VariableResult::Rejected;
            ev_connection_timeout_s = parsed.unwrap();
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "StopTxOnEVSideDisconnect") == 0
         || strcmp(variable, "TxStartPoint") == 0
         || strcmp(variable, "TxStopPoint") == 0)
            return VariableResult::Rejected;
        return VariableResult::UnknownVariable;
    }

    if (strcmp(component, "SampledDataCtrlr") == 0) {
        if (strcmp(variable, "TxUpdatedInterval") == 0) {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() < 0)
                return VariableResult::Rejected;
            tx_updated_interval_s = parsed.unwrap();
            return VariableResult::Accepted;
        }
        return VariableResult::UnknownVariable;
    }

    if (strcmp(component, "AuthCtrlr") == 0) {
        if (strcmp(variable, "AuthorizeRemoteStart") == 0)
            return VariableResult::Rejected;
        return VariableResult::UnknownVariable;
    }

    if (strcmp(component, "SecurityCtrlr") == 0) {
        if (strcmp(variable, "BasicAuthPassword") == 0) {
            size_t len = strlen(value);
            if (len < OCPP21_BASIC_AUTH_PASSWORD_MIN_LEN || len > OCPP21_BASIC_AUTH_PASSWORD_MAX_LEN)
                return VariableResult::Rejected;
            memcpy(basic_auth_password, value, len + 1);
            basic_auth_password_changed = true;
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "OrganizationName") == 0) {
            size_t len = strlen(value);
            if (len == 0 || len > OCPP21_ORGANIZATION_NAME_MAX_LEN)
                return VariableResult::Rejected;
            memcpy(organization_name, value, len + 1);
            organization_name_changed = true;
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "SecurityProfile") == 0
         || strcmp(variable, "Identity") == 0
         || strcmp(variable, "CertificateEntries") == 0)
            return VariableResult::Rejected;
        return VariableResult::UnknownVariable;
    }

    return VariableResult::UnknownComponent;
}

} // namespace Ocpp21
