#pragma once

#include <stdint.h>
#include <stddef.h>

namespace Ocpp21 {

// Minimal device model:
// a flat compile time variable list behind the Component/Variable wire API.
// Attributes are limited to Actual. Extended on demand.

#define OCPP21_BASIC_AUTH_PASSWORD_MIN_LEN 16
#define OCPP21_BASIC_AUTH_PASSWORD_MAX_LEN 64
#define OCPP21_ORGANIZATION_NAME_MAX_LEN 64
#define OCPP21_SECC_ID_MAX_LEN 64
#define OCPP21_COUNTRY_NAME_LEN 2

enum class VariableResult : uint8_t {
    Accepted,
    Rejected,
    UnknownComponent,
    UnknownVariable,
    NotSupportedAttributeType,
};

class CertStore21;

class DeviceModel21 {
public:
    // Values owned by the device model.
    int32_t heartbeat_interval_s = 300;
    int32_t ev_connection_timeout_s = 60;
    int32_t tx_updated_interval_s = 60;

    // SecurityCtrlr. The password is write only on the wire and validated
    // to 16 to 64 characters. identity is not owned, set by the charge
    // point to its name.
    int32_t security_profile = 1;
    const char *identity = nullptr;
    char basic_auth_password[OCPP21_BASIC_AUTH_PASSWORD_MAX_LEN + 1] = "";
    char organization_name[OCPP21_ORGANIZATION_NAME_MAX_LEN + 1] = "";

    // SecurityCtrlr, certificate management. The chain size limit
    // matches the CertificateSigned certificateChain field size.
    int32_t max_certificate_chain_size = 10000;
    int32_t cert_signing_wait_minimum_s = 30;
    int32_t cert_signing_repeat_times = 2;
    // CertificateEntries, set by the charge point.
    const CertStore21 *cert_store = nullptr;

    // ISO15118Ctrlr, subject fields of the SECC leaf CSR. Must match the
    // SECC registration at the V2G PKI operator.
    char secc_id[OCPP21_SECC_ID_MAX_LEN + 1] = "DE*TFO*E000001";
    char country_name[OCPP21_COUNTRY_NAME_LEN + 1] = "DE";
    char iso_organization_name[OCPP21_ORGANIZATION_NAME_MAX_LEN + 1] = "Tinkerforge GmbH";
    // V2G20SECCLeafCryptoSuite: ecdsa_secp521r1_sha512 (default) or ed448.
    bool v2g20_use_ed448 = false;

    // Set on accepted writes, cleared by the charge point after it has
    // persisted the value and applied side effects.
    bool basic_auth_password_changed = false;
    bool organization_name_changed = false;
    bool heartbeat_interval_changed = false;
    bool iso15118_changed = false;

    // On success writes the value as string into buf.
    VariableResult getVariable(const char *component, const char *variable, char *buf, size_t buf_len);
    VariableResult setVariable(const char *component, const char *variable, const char *value);
};

} // namespace Ocpp21
