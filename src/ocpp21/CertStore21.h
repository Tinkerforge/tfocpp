#pragma once

#include <stdint.h>
#include <time.h>

#include <memory>
#include <string>
#include <vector>

#include "Platform21.h"

namespace Ocpp21 {

// Public charging PKI ecosystems require capacity for at least 30 V2G, 50 OEM and 40 MO root certificates.
#define OCPP21_CERTSTORE_MAX_V2G_ROOT 30
#define OCPP21_CERTSTORE_MAX_MO_ROOT 40
#define OCPP21_CERTSTORE_MAX_OEM_ROOT 50
#define OCPP21_CERTSTORE_MAX_CSMS_ROOT 4
#define OCPP21_CERTSTORE_MAX_MFR_ROOT 4
// SECC chains from different V2G roots in parallel, plus the CSMS client chain.
#define OCPP21_CERTSTORE_MAX_CHAINS 4

// CertificateSigned certificateChain is at most 10000 characters.
#define OCPP21_CERT_PEM_MAX 10000
// A root certificate is a single PEM certificate.
#define OCPP21_ROOT_PEM_MAX 4000
// Sub CAs per chain (SECC: CPO sub CA 1 and 2).
#define OCPP21_CHAIN_MAX_CHILDREN 3

enum class CertGroup21 : uint8_t {
    V2GRoot,
    MORoot,
    OEMRoot,
    CsmsRoot,
    MfrRoot,
    CsmsClientChain,
    V2GChain,   // ISO 15118-2 SECC chain
    V2G20Chain, // ISO 15118-20 SECC chain
    None,
};

struct CertEntry21 {
    CertGroup21 group = CertGroup21::None;
    uint32_t id = 0;
    // Root or chain leaf.
    OcppCertHashData21 hash{};
    time_t not_before = 0;
    time_t not_after = 0;
    // Chains only: sub CA hash data in chain order (CPO sub CA 2, then
    // CPO sub CA 1) and the anchoring root for A03 renewals.
    uint8_t child_count = 0;
    OcppCertHashData21 child_hash[OCPP21_CHAIN_MAX_CHILDREN]{};
    OcppCertHashData21 anchor_root{};
    bool has_anchor = false;
};

enum class CertInstallResult21 : uint8_t {
    Accepted,
    Rejected,
    Failed,
};

enum class CertDeleteResult21 : uint8_t {
    Accepted,
    Failed,
    NotFound,
};

class CertStore21 {
public:
    // Scans <charge_point_name>.certs. Roots are loaded first so that
    // chain entries can recover their anchor root.
    void init(const char *charge_point_name);

    CertInstallResult21 installRoot(CertGroup21 group, const char *pem);
    // The chain file and key file ids are reserved by the caller via
    // nextId (the key is written at CSR time). Replaces chains of the
    // same group anchored at the same root (HUB20-42-002, A02.FR.13).
    bool installChain(CertGroup21 group, uint32_t id, const char *pem, const OcppCertHashData21 &anchor_root);
    CertDeleteResult21 deleteByHash(const char *issuer_name_hash, const char *issuer_key_hash, const char *serial_number);
    void removeChain(uint32_t id);

    size_t count() const { return entries.size(); }
    const std::vector<CertEntry21> &all() const { return entries; }
    const CertEntry21 *find(CertGroup21 group) const;
    const CertEntry21 *findById(uint32_t id) const;

    uint32_t nextId() { return next_id++; }

    std::string pemPath(CertGroup21 group, uint32_t id) const;
    std::string keyPath(uint32_t id) const;
    size_t readPem(const CertEntry21 &e, char *buf, size_t buf_len) const;

    // Loads all root PEMs of a group. bufs and ptrs must have space for
    // max entries. Returns the number of loaded roots.
    size_t loadRoots(CertGroup21 group, std::unique_ptr<char[]> *bufs, const char **ptrs, size_t max) const;
    // The root PEM matching hash data, empty if not installed.
    std::string loadRootByHash(const OcppCertHashData21 &hash) const;

private:
    bool addEntry(CertGroup21 group, uint32_t id, const char *pem);
    size_t groupCount(CertGroup21 group) const;
    size_t groupLimit(CertGroup21 group) const;

    std::vector<CertEntry21> entries;
    std::string dir;
    uint32_t next_id = 1;
};

} // namespace Ocpp21
