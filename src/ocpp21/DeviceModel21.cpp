#include "DeviceModel21.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <common/Tools.h>

#include "CertStore21.h"

namespace Ocpp21 {

enum VarId : size_t {
    VAR_HEARTBEAT_INTERVAL,
    VAR_NETWORK_CONFIGURATION_PRIORITY,
    VAR_MESSAGE_ATTEMPTS,
    VAR_MESSAGE_ATTEMPT_INTERVAL,
    VAR_EV_CONNECTION_TIMEOUT,
    VAR_STOP_TX_ON_EV_SIDE_DISCONNECT,
    VAR_TX_START_POINT,
    VAR_TX_STOP_POINT,
    VAR_TX_UPDATED_INTERVAL,
    VAR_AUTHORIZE_REMOTE_START,
    VAR_ITEMS_PER_MESSAGE_GET_REPORT,
    VAR_ITEMS_PER_MESSAGE_GET_VARIABLES,
    VAR_ITEMS_PER_MESSAGE_SET_VARIABLES,
    VAR_BYTES_PER_MESSAGE_GET_REPORT,
    VAR_BYTES_PER_MESSAGE_GET_VARIABLES,
    VAR_BYTES_PER_MESSAGE_SET_VARIABLES,
    VAR_SECURITY_PROFILE,
    VAR_IDENTITY,
    VAR_SEC_ORGANIZATION_NAME,
    VAR_CERTIFICATE_ENTRIES,
    VAR_MAX_CERTIFICATE_CHAIN_SIZE,
    VAR_CERT_SIGNING_WAIT_MINIMUM,
    VAR_CERT_SIGNING_REPEAT_TIMES,
    VAR_BASIC_AUTH_PASSWORD,
    VAR_SECC_ID,
    VAR_COUNTRY_NAME,
    VAR_ISO_ORGANIZATION_NAME,
    VAR_V2G20_SECC_LEAF_CRYPTO_SUITE,
    VAR_COUNT
};

// The report mask in ChargePoint is a uint64_t.
static_assert(VAR_COUNT <= 64, "report mask width exceeded");

// Must match the VarId order.
static const VariableDesc variable_descs[VAR_COUNT] = {
    {"OCPPCommCtrlr", "HeartbeatInterval",            nullptr,            VariableDataType::Integer,      VariableMutability::ReadWrite, false, false, -1,    nullptr, "s"},
    {"OCPPCommCtrlr", "NetworkConfigurationPriority", nullptr,            VariableDataType::SequenceList, VariableMutability::ReadWrite, true,  false, 1,     "1,2,3,4", nullptr},
    {"OCPPCommCtrlr", "MessageAttempts",              "TransactionEvent", VariableDataType::Integer,      VariableMutability::ReadWrite, false, false, -1,    nullptr, nullptr},
    {"OCPPCommCtrlr", "MessageAttemptInterval",       "TransactionEvent", VariableDataType::Integer,      VariableMutability::ReadWrite, false, false, -1,    nullptr, "s"},
    {"TxCtrlr",       "EVConnectionTimeOut",          nullptr,            VariableDataType::Integer,      VariableMutability::ReadWrite, false, false, -1,    nullptr, "s"},
    {"TxCtrlr",       "StopTxOnEVSideDisconnect",     nullptr,            VariableDataType::Boolean,      VariableMutability::ReadOnly,  false, true,  -1,    nullptr, nullptr},
    {"TxCtrlr",       "TxStartPoint",                 nullptr,            VariableDataType::MemberList,   VariableMutability::ReadOnly,  false, true,  -1,    "PowerPathClosed", nullptr},
    {"TxCtrlr",       "TxStopPoint",                  nullptr,            VariableDataType::MemberList,   VariableMutability::ReadOnly,  false, true,  -1,    "PowerPathClosed", nullptr},
    {"SampledDataCtrlr", "TxUpdatedInterval",         nullptr,            VariableDataType::Integer,      VariableMutability::ReadWrite, false, false, -1,    nullptr, "s"},
    {"AuthCtrlr",     "AuthorizeRemoteStart",         nullptr,            VariableDataType::Boolean,      VariableMutability::ReadOnly,  false, true,  -1,    nullptr, nullptr},
    {"DeviceDataCtrlr", "ItemsPerMessage",            "GetReport",        VariableDataType::Integer,      VariableMutability::ReadOnly,  false, true,  -1,    nullptr, nullptr},
    {"DeviceDataCtrlr", "ItemsPerMessage",            "GetVariables",     VariableDataType::Integer,      VariableMutability::ReadOnly,  false, true,  -1,    nullptr, nullptr},
    {"DeviceDataCtrlr", "ItemsPerMessage",            "SetVariables",     VariableDataType::Integer,      VariableMutability::ReadOnly,  false, true,  -1,    nullptr, nullptr},
    {"DeviceDataCtrlr", "BytesPerMessage",            "GetReport",        VariableDataType::Integer,      VariableMutability::ReadOnly,  false, true,  -1,    nullptr, nullptr},
    {"DeviceDataCtrlr", "BytesPerMessage",            "GetVariables",     VariableDataType::Integer,      VariableMutability::ReadOnly,  false, true,  -1,    nullptr, nullptr},
    {"DeviceDataCtrlr", "BytesPerMessage",            "SetVariables",     VariableDataType::Integer,      VariableMutability::ReadOnly,  false, true,  -1,    nullptr, nullptr},
    {"SecurityCtrlr", "SecurityProfile",              nullptr,            VariableDataType::Integer,      VariableMutability::ReadOnly,  true,  false, -1,    nullptr, nullptr},
    {"SecurityCtrlr", "Identity",                     nullptr,            VariableDataType::String,       VariableMutability::ReadOnly,  true,  false, -1,    nullptr, nullptr},
    {"SecurityCtrlr", "OrganizationName",             nullptr,            VariableDataType::String,       VariableMutability::ReadWrite, true,  false, OCPP21_ORGANIZATION_NAME_MAX_LEN, nullptr, nullptr},
    // HUB20-411-006..008: capacity for 30 V2G, 50 OEM and 40 MO roots.
    {"SecurityCtrlr", "CertificateEntries",           nullptr,            VariableDataType::Integer,      VariableMutability::ReadOnly,  true,  false, OCPP21_CERTSTORE_MAX_ENTRIES, nullptr, nullptr},
    {"SecurityCtrlr", "MaxCertificateChainSize",      nullptr,            VariableDataType::Integer,      VariableMutability::ReadOnly,  false, true,  -1,    nullptr, nullptr},
    {"SecurityCtrlr", "CertSigningWaitMinimum",       nullptr,            VariableDataType::Integer,      VariableMutability::ReadWrite, false, false, -1,    nullptr, "s"},
    {"SecurityCtrlr", "CertSigningRepeatTimes",       nullptr,            VariableDataType::Integer,      VariableMutability::ReadWrite, false, false, -1,    nullptr, nullptr},
    // A00.FR.304: maxLimit at least 40, at most 64.
    {"SecurityCtrlr", "BasicAuthPassword",            nullptr,            VariableDataType::String,       VariableMutability::WriteOnly, true,  false, OCPP21_BASIC_AUTH_PASSWORD_MAX_LEN, nullptr, nullptr},
    {"ISO15118Ctrlr", "SeccId",                       nullptr,            VariableDataType::String,       VariableMutability::ReadWrite, true,  false, OCPP21_SECC_ID_MAX_LEN, nullptr, nullptr},
    {"ISO15118Ctrlr", "CountryName",                  nullptr,            VariableDataType::String,       VariableMutability::ReadWrite, true,  false, OCPP21_COUNTRY_NAME_LEN, nullptr, nullptr},
    {"ISO15118Ctrlr", "OrganizationName",             nullptr,            VariableDataType::String,       VariableMutability::ReadWrite, true,  false, OCPP21_ORGANIZATION_NAME_MAX_LEN, nullptr, nullptr},
    {"ISO15118Ctrlr", "V2G20SECCLeafCryptoSuite",     nullptr,            VariableDataType::OptionList,   VariableMutability::ReadWrite, true,  false, -1,    "ecdsa_secp521r1_sha512,ed448", nullptr},
};

size_t DeviceModel::variableCount()
{
    return VAR_COUNT;
}

const VariableDesc &DeviceModel::variableDesc(size_t idx)
{
    return variable_descs[idx];
}

// Component and variable names are case insensitive on the wire. A
// request without an instance matches an instanced variable, the
// variable names are unique either way.
static bool instance_matches(const VariableDesc &desc, const char *instance)
{
    if (instance == nullptr || instance[0] == '\0')
        return true;
    return desc.instance != nullptr && strcasecmp(desc.instance, instance) == 0;
}

static VariableResult find_variable(const char *component, const char *variable, const char *instance, size_t *idx_out)
{
    bool component_known = false;
    for (size_t i = 0; i < VAR_COUNT; ++i) {
        const auto &desc = variable_descs[i];
        if (strcasecmp(desc.component, component) != 0)
            continue;
        component_known = true;
        if (strcasecmp(desc.variable, variable) != 0)
            continue;
        if (!instance_matches(desc, instance))
            continue;
        *idx_out = i;
        return VariableResult::Accepted;
    }
    return component_known ? VariableResult::UnknownVariable : VariableResult::UnknownComponent;
}

VariableResult DeviceModel::getVariableByIndex(size_t idx, char *buf, size_t buf_len)
{
    switch (idx) {
        case VAR_HEARTBEAT_INTERVAL:
            snprintf(buf, buf_len, "%d", heartbeat_interval_s);
            return VariableResult::Accepted;
        case VAR_NETWORK_CONFIGURATION_PRIORITY:
            if (network_priority == 0) {
                buf[0] = '\0';
            } else {
                snprintf(buf, buf_len, "%d", network_priority);
            }
            return VariableResult::Accepted;
        case VAR_MESSAGE_ATTEMPTS:
            snprintf(buf, buf_len, "%d", message_attempts);
            return VariableResult::Accepted;
        case VAR_MESSAGE_ATTEMPT_INTERVAL:
            snprintf(buf, buf_len, "%d", message_attempt_interval_s);
            return VariableResult::Accepted;
        case VAR_EV_CONNECTION_TIMEOUT:
            snprintf(buf, buf_len, "%d", ev_connection_timeout_s);
            return VariableResult::Accepted;
        case VAR_STOP_TX_ON_EV_SIDE_DISCONNECT:
            snprintf(buf, buf_len, "true");
            return VariableResult::Accepted;
        case VAR_TX_START_POINT:
        case VAR_TX_STOP_POINT:
            snprintf(buf, buf_len, "PowerPathClosed");
            return VariableResult::Accepted;
        case VAR_TX_UPDATED_INTERVAL:
            snprintf(buf, buf_len, "%d", tx_updated_interval_s);
            return VariableResult::Accepted;
        case VAR_AUTHORIZE_REMOTE_START:
            snprintf(buf, buf_len, "false");
            return VariableResult::Accepted;
        case VAR_ITEMS_PER_MESSAGE_GET_REPORT:
        case VAR_ITEMS_PER_MESSAGE_GET_VARIABLES:
        case VAR_ITEMS_PER_MESSAGE_SET_VARIABLES:
            snprintf(buf, buf_len, "%d", OCPP21_ITEMS_PER_MESSAGE);
            return VariableResult::Accepted;
        case VAR_BYTES_PER_MESSAGE_GET_REPORT:
        case VAR_BYTES_PER_MESSAGE_GET_VARIABLES:
        case VAR_BYTES_PER_MESSAGE_SET_VARIABLES:
            snprintf(buf, buf_len, "%d", OCPP21_BYTES_PER_MESSAGE);
            return VariableResult::Accepted;
        case VAR_SECURITY_PROFILE:
            snprintf(buf, buf_len, "%d", security_profile);
            return VariableResult::Accepted;
        case VAR_IDENTITY:
            if (identity == nullptr)
                return VariableResult::Rejected;
            snprintf(buf, buf_len, "%s", identity);
            return VariableResult::Accepted;
        case VAR_SEC_ORGANIZATION_NAME:
            snprintf(buf, buf_len, "%s", organization_name);
            return VariableResult::Accepted;
        case VAR_CERTIFICATE_ENTRIES:
            snprintf(buf, buf_len, "%u", cert_store != nullptr ? (unsigned)cert_store->count() : 0u);
            return VariableResult::Accepted;
        case VAR_MAX_CERTIFICATE_CHAIN_SIZE:
            snprintf(buf, buf_len, "%d", max_certificate_chain_size);
            return VariableResult::Accepted;
        case VAR_CERT_SIGNING_WAIT_MINIMUM:
            snprintf(buf, buf_len, "%d", cert_signing_wait_minimum_s);
            return VariableResult::Accepted;
        case VAR_CERT_SIGNING_REPEAT_TIMES:
            snprintf(buf, buf_len, "%d", cert_signing_repeat_times);
            return VariableResult::Accepted;
        case VAR_BASIC_AUTH_PASSWORD:
            // WriteOnly, reads are rejected (B06.FR.09).
            return VariableResult::Rejected;
        case VAR_SECC_ID:
            snprintf(buf, buf_len, "%s", secc_id);
            return VariableResult::Accepted;
        case VAR_COUNTRY_NAME:
            snprintf(buf, buf_len, "%s", country_name);
            return VariableResult::Accepted;
        case VAR_ISO_ORGANIZATION_NAME:
            snprintf(buf, buf_len, "%s", iso_organization_name);
            return VariableResult::Accepted;
        case VAR_V2G20_SECC_LEAF_CRYPTO_SUITE:
            snprintf(buf, buf_len, "%s", v2g20_use_ed448 ? "ed448" : "ecdsa_secp521r1_sha512");
            return VariableResult::Accepted;
    }
    return VariableResult::UnknownVariable;
}

VariableResult DeviceModel::getVariable(const char *component, const char *variable, const char *instance, char *buf, size_t buf_len)
{
    size_t idx;
    VariableResult found = find_variable(component, variable, instance, &idx);
    if (found != VariableResult::Accepted)
        return found;
    return getVariableByIndex(idx, buf, buf_len);
}

VariableResult DeviceModel::setVariable(const char *component, const char *variable, const char *instance, const char *value)
{
    size_t idx;
    VariableResult found = find_variable(component, variable, instance, &idx);
    if (found != VariableResult::Accepted)
        return found;

    switch (idx) {
        case VAR_HEARTBEAT_INTERVAL: {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() <= 0)
                return VariableResult::Rejected;
            heartbeat_interval_s = parsed.unwrap();
            heartbeat_interval_changed = true;
            return VariableResult::Accepted;
        }
        case VAR_NETWORK_CONFIGURATION_PRIORITY: {
            // A05: a single slot, no B10 fallback list yet. The slot must
            // hold a profile stored via SetNetworkProfile.
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() < 1 || parsed.unwrap() > OCPP21_NETWORK_PROFILE_SLOTS)
                return VariableResult::Rejected;
            if (!network_profiles[parsed.unwrap() - 1].used)
                return VariableResult::Rejected;
            network_priority = parsed.unwrap();
            network_priority_changed = true;
            return VariableResult::Accepted;
        }
        case VAR_MESSAGE_ATTEMPTS: {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() < 1)
                return VariableResult::Rejected;
            message_attempts = parsed.unwrap();
            return VariableResult::Accepted;
        }
        case VAR_MESSAGE_ATTEMPT_INTERVAL: {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() < 0)
                return VariableResult::Rejected;
            message_attempt_interval_s = parsed.unwrap();
            return VariableResult::Accepted;
        }
        case VAR_EV_CONNECTION_TIMEOUT: {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() < 0)
                return VariableResult::Rejected;
            ev_connection_timeout_s = parsed.unwrap();
            return VariableResult::Accepted;
        }
        case VAR_TX_UPDATED_INTERVAL: {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() < 0)
                return VariableResult::Rejected;
            tx_updated_interval_s = parsed.unwrap();
            return VariableResult::Accepted;
        }
        case VAR_SEC_ORGANIZATION_NAME: {
            size_t len = strlen(value);
            if (len == 0 || len > OCPP21_ORGANIZATION_NAME_MAX_LEN)
                return VariableResult::Rejected;
            memcpy(organization_name, value, len + 1);
            organization_name_changed = true;
            return VariableResult::Accepted;
        }
        case VAR_CERT_SIGNING_WAIT_MINIMUM: {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() <= 0)
                return VariableResult::Rejected;
            cert_signing_wait_minimum_s = parsed.unwrap();
            return VariableResult::Accepted;
        }
        case VAR_CERT_SIGNING_REPEAT_TIMES: {
            auto parsed = parse_int(value);
            if (parsed.is_none() || parsed.unwrap() < 0)
                return VariableResult::Rejected;
            cert_signing_repeat_times = parsed.unwrap();
            return VariableResult::Accepted;
        }
        case VAR_BASIC_AUTH_PASSWORD: {
            size_t len = strlen(value);
            if (len < OCPP21_BASIC_AUTH_PASSWORD_MIN_LEN || len > OCPP21_BASIC_AUTH_PASSWORD_MAX_LEN)
                return VariableResult::Rejected;
            memcpy(basic_auth_password, value, len + 1);
            basic_auth_password_changed = true;
            return VariableResult::Accepted;
        }
        case VAR_SECC_ID: {
            size_t len = strlen(value);
            if (len < 7 || len > OCPP21_SECC_ID_MAX_LEN)
                return VariableResult::Rejected;
            memcpy(secc_id, value, len + 1);
            iso15118_changed = true;
            return VariableResult::Accepted;
        }
        case VAR_COUNTRY_NAME: {
            if (strlen(value) != OCPP21_COUNTRY_NAME_LEN)
                return VariableResult::Rejected;
            memcpy(country_name, value, OCPP21_COUNTRY_NAME_LEN + 1);
            iso15118_changed = true;
            return VariableResult::Accepted;
        }
        case VAR_ISO_ORGANIZATION_NAME: {
            size_t len = strlen(value);
            if (len == 0 || len > OCPP21_ORGANIZATION_NAME_MAX_LEN)
                return VariableResult::Rejected;
            memcpy(iso_organization_name, value, len + 1);
            iso15118_changed = true;
            return VariableResult::Accepted;
        }
        case VAR_V2G20_SECC_LEAF_CRYPTO_SUITE: {
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
    }
    return VariableResult::Rejected;
}

} // namespace Ocpp21
