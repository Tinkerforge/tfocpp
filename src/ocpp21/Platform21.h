#pragma once

// OCPP 2.1 specific platform hooks. The shared platform API lives in
// common/Platform.h. The 21 suffix on names avoids collisions with the
// 1.6 hooks in ocpp16/Platform16.h when both stacks are linked into one
// binary.

#include <stdint.h>
#include <stddef.h>
#include <time.h>

enum class EvseState21 : uint8_t {
    NotConnected,  // no cable plugged
    PlugDetected,  // cable plugged into the EVSE, no EV connected
    Connected,     // EV connected, energy flow not released by the EVSE
    ReadyToCharge, // energy flow released, EV does not draw energy
    Charging,      // energy flows
    Faulted,
};

// evse_id is the EVSE the tag reader belongs to, 0 for a station wide reader.
void platform_register_tag_seen_callback21(void *ctx, void (*cb)(int32_t evse_id, const char *tag_id, void *user_data), void *user_data);

EvseState21 platform_get_evse_state21(void *ctx, int32_t evse_id);

// Close (true) or open (false) the power path of the EVSE.
void platform_set_charging_allowed21(void *ctx, int32_t evse_id, bool allowed);

// Energy.Active.Import.Register of the EVSE meter in Wh.
float platform_get_energy_wh21(void *ctx, int32_t evse_id);

// Certificate and key primitives for the certificate management use
// cases. PEM inputs may contain multiple concatenated certificates (a
// chain, leaf first), idx selects one certificate within the PEM.
// Implemented with OpenSSL on the Linux host and mbedTLS on the ESP32.

struct OcppCertInfo21 {
    time_t not_before;
    time_t not_after;
    bool is_ca;
    bool self_signed;
};

// SHA256 hex, lower case, null terminated (A00.FR.506).
struct OcppCertHashData21 {
    char issuer_name_hash[65];
    char issuer_key_hash[65];
    // Hex without leading zero octets, up to 20 octets.
    char serial_number[41];
};

// Number of certificates in the PEM bundle, 0 if unparseable.
size_t platform_cert_count21(const char *pem);

bool platform_cert_info21(const char *pem, size_t idx, OcppCertInfo21 *info);

// Hash data of pem[idx]. issuer_pem[issuer_idx] provides the issuer key
// for issuerKeyHash, issuer_pem == nullptr uses the certificate itself
// (self signed roots).
bool platform_cert_hash_data21(const char *pem, size_t idx, const char *issuer_pem, size_t issuer_idx, OcppCertHashData21 *out);

enum class OcppChainVerifyResult21 : uint8_t {
    Ok,
    NotYetValid,
    Expired,
    Untrusted, // no path to any of the given roots
    Invalid,   // parse error or broken signature
};

// Verifies chain_pem (leaf first, then intermediates) against the given
// roots at time now. On Ok anchor_idx (if not null) receives the index
// of the root that anchored the chain.
OcppChainVerifyResult21 platform_verify_chain21(const char *chain_pem, const char * const *roots_pem, size_t roots_len, time_t now, size_t *anchor_idx);

enum class OcppCurve21 : uint8_t {
    Secp256r1,
    Secp521r1,
    Ed448,
};

struct OcppCsrParams21 {
    OcppCurve21 curve;
    const char *common_name;
    const char *organization; // nullptr to omit
    const char *country;      // nullptr to omit
    const char *key_name;     // platform storage name for the new private key
};

// Generates a new key pair and a PEM encoded CSR (RFC 2986). The
// private key is stored by the platform under key_name and never
// crosses this API (A02.FR.05). Returns the CSR length or 0 on error.
size_t platform_generate_csr21(const OcppCsrParams21 *params, char *csr_pem, size_t csr_pem_len);

// True if the leaf (idx 0) of cert_pem matches the private key stored
// under key_name.
bool platform_key_matches_cert21(const char *key_name, const char *cert_pem);

enum class OcppOcspStatus21 : uint8_t {
    Good,
    Revoked,
    Unknown, // the responder does not know the certificate
    Invalid, // response validation failed (RFC 6960)
};

// Validates a base64 encoded DER OCSP response for pem[idx] (issued by
// issuer_pem[issuer_idx]) per RFC 6960 including the responder
// certificate chain up to one of roots_pem. next_update (if not null)
// receives the nextUpdate time, 0 if the response has none.
OcppOcspStatus21 platform_ocsp_validate21(const char *ocsp_response_b64,
                                          const char *pem, size_t idx,
                                          const char *issuer_pem, size_t issuer_idx,
                                          const char * const *roots_pem, size_t roots_len,
                                          time_t now, time_t *next_update);

// OCSP responder URL from the authority information access extension.
bool platform_cert_ocsp_url21(const char *pem, size_t idx, char *url, size_t url_len);
