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

// CertificateEntries.maxLimit (HUB20-411-005).
#define OCPP21_CERTSTORE_MAX_ENTRIES (OCPP21_CERTSTORE_MAX_V2G_ROOT + OCPP21_CERTSTORE_MAX_MO_ROOT + OCPP21_CERTSTORE_MAX_OEM_ROOT + OCPP21_CERTSTORE_MAX_CSMS_ROOT + OCPP21_CERTSTORE_MAX_MFR_ROOT + OCPP21_CERTSTORE_MAX_CHAINS)

// CertificateSigned certificateChain is at most 10000 characters.
#define OCPP21_CERT_PEM_MAX 10000
// A root certificate is a single PEM certificate.
#define OCPP21_ROOT_PEM_MAX OCPP21_CERT_PEM_MAX
// Sub CAs per chain (SECC: CPO sub CA 1 and 2).
#define OCPP21_CHAIN_MAX_CHILDREN 3

enum class CertGroup : uint8_t {
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

struct CertEntry {
    CertGroup group = CertGroup::None;
    uint32_t id = 0;
    // Root or chain leaf.
    OcppCertHashData21 hash{};
    time_t not_before = 0;
    time_t not_after = 0;
    OcppCurve21 public_key_curve = OcppCurve21::Unknown;
    // Chains only: sub CA hash data in chain order (CPO sub CA 2, then
    // CPO sub CA 1) and the anchoring root for A03 renewals.
    uint8_t child_count = 0;
    OcppCertHashData21 child_hash[OCPP21_CHAIN_MAX_CHILDREN]{};
    OcppCertHashData21 anchor_root{};
    bool has_anchor = false;
};

enum class CertInstallResult : uint8_t {
    Accepted,
    Rejected,
    Failed,
};

enum class ChainInstallResult : uint8_t {
    Installed,
    RetainedExisting,
    Failed,
};

enum class CertDeleteResult : uint8_t {
    Accepted,
    Failed,
    NotFound,
};

class CertStore {
public:
    // Scans <charge_point_name>.certs. Roots are loaded first so that
    // chain entries can recover their anchor root.
    void init(const char *charge_point_name);

    CertInstallResult installRoot(CertGroup group, const char *pem, time_t now);
    // The chain file and key file ids are reserved by the caller via
    // nextId (the key is written at CSR time). SECC chains are unique per
    // root and suite, retaining the newest validity start (HUB20-42-002,
    // A02.FR.15). retain_replaced stages a CSMS client replacement until
    // the caller confirms the new credential connected successfully.
    ChainInstallResult installChain(CertGroup group, uint32_t id, const char *pem,
                                    const OcppCertHashData21 &anchor_root, time_t now,
                                    bool combined = false, bool retain_replaced = false);
    CertDeleteResult deleteByHash(const char *issuer_name_hash, const char *issuer_key_hash, const char *serial_number);
    void removeChain(CertGroup group, uint32_t id);

    // Number reported through SecurityCtrlr.CertificateEntries. A combined
    // CSMS/V2G chain has two logical entries but one physical credential.
    size_t count() const;
    const std::vector<CertEntry> &all() const { return entries; }
    const CertEntry *find(CertGroup group) const;
    const CertEntry *findById(uint32_t id) const;
    const CertEntry *findSeccChainById(uint32_t id) const;

    // Reserves and returns an unused persistent object ID, or zero when none
    // is available. IDs retained by quarantined files are not reused.
    uint32_t nextId();

    std::string pemPath(CertGroup group, uint32_t id) const;
    std::string keyPath(uint32_t id) const;
    size_t readPem(const CertEntry &e, char *buf, size_t buf_len) const;

    // Loads all root PEMs of a group. bufs and ptrs must have space for
    // max entries. Returns the number of loaded roots.
    size_t loadRoots(CertGroup group, std::unique_ptr<char[]> *bufs, const char **ptrs, size_t max) const;
    // The root PEM matching hash data, empty if not installed.
    std::string loadRootByHash(const OcppCertHashData21 &hash) const;

private:
    bool addEntry(CertGroup group, uint32_t id, const char *pem, bool require_anchor = false,
                  const OcppCertHashData21 *known_anchor = nullptr, bool retain_replaced = false);
    size_t groupCount(CertGroup group) const;
    size_t groupLimit(CertGroup group) const;
    size_t chainCredentialCount() const;
    bool idReserved(uint32_t id) const;
    void reserveId(uint32_t id);

    std::vector<CertEntry> entries;
    std::vector<uint32_t> reserved_ids;
    std::string dir;
    uint32_t next_id = 1;
};

} // namespace Ocpp21
