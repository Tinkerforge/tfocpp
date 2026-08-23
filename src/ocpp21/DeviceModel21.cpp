#include "DeviceModel21.h"

#include <stdio.h>
#include <string.h>

#include <common/Tools.h>

#include "CertStore21.h"

namespace Ocpp21 {

VariableResult DeviceModel::getVariable(const char *component, const char *variable, char *buf, size_t buf_len)
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
            snprintf(buf, buf_len, "%u", cert_store != nullptr ? (unsigned)cert_store->count() : 0u);
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "MaxCertificateChainSize") == 0) {
            snprintf(buf, buf_len, "%d", max_certificate_chain_size);
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "CertSigningWaitMinimum") == 0) {
            snprintf(buf, buf_len, "%d", cert_signing_wait_minimum_s);
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "CertSigningRepeatTimes") == 0) {
            snprintf(buf, buf_len, "%d", cert_signing_repeat_times);
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "BasicAuthPassword") == 0) {
            // WriteOnly, reads are rejected (B06.FR.09).
            return VariableResult::Rejected;
        }
        return VariableResult::UnknownVariable;
    }

    if (strcmp(component, "ISO15118Ctrlr") == 0) {
        if (strcmp(variable, "SeccId") == 0) {
            snprintf(buf, buf_len, "%s", secc_id);
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "CountryName") == 0) {
            snprintf(buf, buf_len, "%s", country_name);
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "OrganizationName") == 0) {
            snprintf(buf, buf_len, "%s", iso_organization_name);
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "V2G20SECCLeafCryptoSuite") == 0) {
            snprintf(buf, buf_len, "%s", v2g20_use_ed448 ? "ed448" : "ecdsa_secp521r1_sha512");
            return VariableResult::Accepted;
        }
        return VariableResult::UnknownVariable;
    }

    return VariableResult::UnknownComponent;
}

VariableResult DeviceModel::setVariable(const char *component, const char *variable, const char *value)
{
    if (strcmp(component, "OCPPCommCtrlr") == 0) {
        if (strcmp(variable, "HeartbeatInterval") == 0) {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() <= 0)
                return VariableResult::Rejected;
            heartbeat_interval_s = parsed.unwrap();
            heartbeat_interval_changed = true;
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
        if (strcmp(variable, "CertSigningWaitMinimum") == 0) {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() <= 0) {
                return VariableResult::Rejected;
            }
            cert_signing_wait_minimum_s = parsed.unwrap();
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "CertSigningRepeatTimes") == 0) {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() < 0) {
                return VariableResult::Rejected;
            }
            cert_signing_repeat_times = parsed.unwrap();
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "SecurityProfile") == 0
         || strcmp(variable, "Identity") == 0
         || strcmp(variable, "CertificateEntries") == 0
         || strcmp(variable, "MaxCertificateChainSize") == 0) {
            return VariableResult::Rejected;
        }
        return VariableResult::UnknownVariable;
    }

    if (strcmp(component, "ISO15118Ctrlr") == 0) {
        if (strcmp(variable, "SeccId") == 0) {
            size_t len = strlen(value);
            if (len < 7 || len > OCPP21_SECC_ID_MAX_LEN) {
                return VariableResult::Rejected;
            }
            memcpy(secc_id, value, len + 1);
            iso15118_changed = true;
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "CountryName") == 0) {
            if (strlen(value) != OCPP21_COUNTRY_NAME_LEN) {
                return VariableResult::Rejected;
            }
            memcpy(country_name, value, OCPP21_COUNTRY_NAME_LEN + 1);
            iso15118_changed = true;
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "OrganizationName") == 0) {
            size_t len = strlen(value);
            if (len == 0 || len > OCPP21_ORGANIZATION_NAME_MAX_LEN) {
                return VariableResult::Rejected;
            }
            memcpy(iso_organization_name, value, len + 1);
            iso15118_changed = true;
            return VariableResult::Accepted;
        }
        if (strcmp(variable, "V2G20SECCLeafCryptoSuite") == 0) {
            if (strcmp(value, "ecdsa_secp521r1_sha512") == 0) {
                v2g20_use_ed448 = false;
            } else if (strcmp(value, "ed448") == 0) {
                v2g20_use_ed448 = true;
            } else {
                return VariableResult::Rejected;
            }
            iso15118_changed = true;
            return VariableResult::Accepted;
        }
        return VariableResult::UnknownVariable;
    }

    return VariableResult::UnknownComponent;
}

} // namespace Ocpp21
