#include "CertStore21.h"

#include <algorithm>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <common/Platform.h>

namespace Ocpp21 {

static const char * const group_prefixes[] = {
    "v2gr", "mor", "oemr", "csmsr", "mfrr", "cs", "v2g2", "v2g20",
};

static bool is_chain_group(CertGroup group)
{
    return group == CertGroup::CsmsClientChain || group == CertGroup::V2GChain || group == CertGroup::V2G20Chain;
}

static bool same_hash(const OcppCertHashData21 &a, const OcppCertHashData21 &b)
{
    return strcasecmp(a.issuer_name_hash, b.issuer_name_hash) == 0
        && strcasecmp(a.issuer_key_hash, b.issuer_key_hash) == 0
        && strcasecmp(a.serial_number, b.serial_number) == 0;
}

static bool parse_object_id(const char *text, const char *suffix, uint32_t *id)
{
    if (text == nullptr || *text < '1' || *text > '9') {
        return false;
    }

    errno = 0;
    char *end = nullptr;
    unsigned long value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || strcmp(end, suffix) != 0 || value == 0 || value >= UINT32_MAX) {
        return false;
    }
    *id = static_cast<uint32_t>(value);
    return true;
}

static bool parse_pem_name(const char *name, CertGroup *group, uint32_t *id)
{
    for (size_t g = 0; g < sizeof(group_prefixes) / sizeof(group_prefixes[0]); ++g) {
        const size_t prefix_len = strlen(group_prefixes[g]);
        if (strncmp(name, group_prefixes[g], prefix_len) == 0 && name[prefix_len] == '.'
         && parse_object_id(name + prefix_len + 1, ".pem", id)) {
            *group = static_cast<CertGroup>(g);
            return true;
        }
    }
    return false;
}

static bool parse_key_name(const char *name, uint32_t *id)
{
    return strncmp(name, "key.", 4) == 0 && parse_object_id(name + 4, "", id);
}

static bool read_pem_file(const std::string &path, size_t max_len, std::unique_ptr<char[]> *buf, size_t *len_out = nullptr)
{
    auto data = std::unique_ptr<char[]>(new char[max_len + 2]);
    const size_t len = platform_read_file(path.c_str(), data.get(), max_len + 1);
    if (len == 0 || len > max_len) {
        return false;
    }
    data[len] = '\0';
    if (len_out != nullptr) {
        *len_out = len;
    }
    *buf = std::move(data);
    return true;
}

// This is an application-level store because the ESP-IDF facilities do not
// provide the certificate model required by OCPP and ISO 15118:
// - esp_crt_bundle is a build-time trust-root bundle for server verification.
// - the ESP-TLS global CA store is volatile TLS configuration, not persistence.
// - esp_secure_cert_mgr targets provisioned device credentials and lacks OCPP
//   groups, individual lifecycle operations, chain relationships and OCSP data.
// CertStore owns mutable certificate groups, hashes, chain/key IDs and OCSP bindings.
void CertStore::init(const char *charge_point_name)
{
    dir = std::string(charge_point_name) + ".certs";
    entries.clear();
    reserved_ids.clear();
    next_id = 1;

    struct Found {
        CertGroup group;
        uint32_t id;
    };
    std::vector<Found> found;
    std::vector<uint32_t> key_ids;

    void *d = platform_open_dir(dir.c_str());
    if (d != nullptr) {
        OcppDirEnt *ent;
        while ((ent = platform_read_dir(d)) != nullptr) {
            if (ent->is_dir) {
                continue;
            }
            CertGroup group = CertGroup::None;
            uint32_t id = 0;
            if (!parse_pem_name(ent->name, &group, &id)) {
                if (parse_key_name(ent->name, &id)) {
                    key_ids.push_back(id);
                    reserveId(id);
                }
                continue;
            }
            found.push_back({group, id});
            reserveId(id);
        }
        platform_close_dir(d);
    }

    std::sort(found.begin(), found.end(), [](const Found &a, const Found &b) {
        return a.group != b.group ? a.group < b.group : a.id > b.id;
    });

    // IDs are globally unique except for a combined certificate, which is
    // represented by one CSMS and one ISO 15118-2 chain sharing a key.
    auto valid_id_layout = [&found](uint32_t id) {
        size_t count = 0;
        bool csms = false;
        bool v2g2 = false;
        for (const auto &f : found) {
            if (f.id != id) {
                continue;
            }
            ++count;
            csms |= f.group == CertGroup::CsmsClientChain;
            v2g2 |= f.group == CertGroup::V2GChain;
        }
        return count == 1 || (count == 2 && csms && v2g2);
    };

    // Roots first, chains need them to recover the anchor root. Chains get a
    // second pass so an identical combined pair is retained when only one of
    // its two logical roles has an installed root.
    for (int pass = 0; pass < 3; ++pass) {
        for (auto &f : found) {
            if (is_chain_group(f.group) != (pass != 0)) {
                continue;
            }
            bool already_loaded = false;
            for (const auto &e : entries) {
                if (e.group == f.group && e.id == f.id) {
                    already_loaded = true;
                    break;
                }
            }
            if (already_loaded) {
                continue;
            }
            if (!valid_id_layout(f.id)) {
                log_warn("Certificate store: quarantining ambiguous object ID %u", static_cast<unsigned>(f.id));
                continue;
            }
            if (!is_chain_group(f.group) && groupCount(f.group) >= groupLimit(f.group)) {
                log_warn("Certificate store: ignoring %s.%u above the group limit",
                         group_prefixes[static_cast<size_t>(f.group)], static_cast<unsigned>(f.id));
                continue;
            }
            if (f.group == CertGroup::CsmsClientChain && groupCount(f.group) >= 2) {
                log_warn("Certificate store: ignoring extra staged charging station chain %s.%u",
                         group_prefixes[static_cast<size_t>(f.group)], static_cast<unsigned>(f.id));
                continue;
            }
            const size_t chain_limit = OCPP21_CERTSTORE_MAX_CHAINS + (groupCount(CertGroup::CsmsClientChain) == 2 ? 1 : 0);
            if (is_chain_group(f.group) && findById(f.id) == nullptr
             && chainCredentialCount() >= chain_limit) {
                log_warn("Certificate store: ignoring chain %s.%u above the credential limit",
                         group_prefixes[static_cast<size_t>(f.group)], static_cast<unsigned>(f.id));
                continue;
            }

            std::string path = pemPath(f.group, f.id);
            std::unique_ptr<char[]> buf;
            size_t len = 0;
            const size_t max_len = is_chain_group(f.group) ? OCPP21_CERT_PEM_MAX : OCPP21_ROOT_PEM_MAX;
            if (!read_pem_file(path, max_len, &buf, &len)) {
                log_warn("Certificate store: quarantining unreadable or oversized %s", path.c_str());
                continue;
            }

            const CertEntry *same_id_entry = findById(f.id);
            if (same_id_entry != nullptr) {
                bool combined_pair = is_chain_group(f.group) && is_chain_group(same_id_entry->group)
                                  && ((f.group == CertGroup::CsmsClientChain && same_id_entry->group == CertGroup::V2GChain)
                                   || (f.group == CertGroup::V2GChain && same_id_entry->group == CertGroup::CsmsClientChain));
                std::unique_ptr<char[]> other;
                size_t other_len = 0;
                if (!combined_pair
                 || !read_pem_file(pemPath(same_id_entry->group, same_id_entry->id), OCPP21_CERT_PEM_MAX, &other, &other_len)
                 || len != other_len || memcmp(buf.get(), other.get(), len) != 0) {
                    log_warn("Certificate store: quarantining conflicting object ID %u", static_cast<unsigned>(f.id));
                    continue;
                }
            }

            if (is_chain_group(f.group) && !platform_key_matches_cert21(keyPath(f.id).c_str(), buf.get())) {
                log_warn("Certificate store: quarantining chain %s.%u with a missing or mismatched key",
                         group_prefixes[static_cast<size_t>(f.group)], static_cast<unsigned>(f.id));
                continue;
            }
            const bool require_anchor = is_chain_group(f.group)
                                     && (same_id_entry == nullptr || f.group != CertGroup::CsmsClientChain);
            if (!addEntry(f.group, f.id, buf.get(), require_anchor, nullptr, f.group == CertGroup::CsmsClientChain)) {
                log_warn("Certificate store: failed to load %s", path.c_str());
            }
        }
    }

    // A key without any recognized chain file can only belong to an
    // interrupted CSR. Retain keys next to quarantined chains for diagnosis.
    for (uint32_t id : key_ids) {
        bool has_chain_file = false;
        for (const auto &f : found) {
            if (f.id == id && is_chain_group(f.group)) {
                has_chain_file = true;
                break;
            }
        }
        if (!has_chain_file) {
            platform_remove_file(keyPath(id).c_str());
            bool has_pem_file = false;
            for (const auto &f : found) {
                if (f.id == id) {
                    has_pem_file = true;
                    break;
                }
            }
            if (!has_pem_file) {
                reserved_ids.erase(std::remove(reserved_ids.begin(), reserved_ids.end(), id), reserved_ids.end());
            }
        }
    }

    next_id = 1;
    while (next_id < UINT32_MAX && idReserved(next_id)) {
        ++next_id;
    }
}

size_t CertStore::count() const
{
    size_t result = 0;
    std::vector<uint32_t> chain_ids;
    for (const auto &e : entries) {
        if (!is_chain_group(e.group)) {
            ++result;
        } else if (std::find(chain_ids.begin(), chain_ids.end(), e.id) == chain_ids.end()) {
            chain_ids.push_back(e.id);
            ++result;
        }
    }
    return result;
}

size_t CertStore::chainCredentialCount() const
{
    size_t result = 0;
    std::vector<uint32_t> ids;
    for (const auto &e : entries) {
        if (is_chain_group(e.group) && std::find(ids.begin(), ids.end(), e.id) == ids.end()) {
            ids.push_back(e.id);
            ++result;
        }
    }
    return result;
}

bool CertStore::idReserved(uint32_t id) const
{
    return std::find(reserved_ids.begin(), reserved_ids.end(), id) != reserved_ids.end();
}

void CertStore::reserveId(uint32_t id)
{
    if (id != 0 && id != UINT32_MAX && !idReserved(id)) {
        reserved_ids.push_back(id);
    }
}

uint32_t CertStore::nextId()
{
    if (next_id == 0 || next_id == UINT32_MAX) {
        next_id = 1;
    }
    const uint32_t start = next_id;
    do {
        if (!idReserved(next_id)) {
            const uint32_t result = next_id;
            reserveId(result);
            ++next_id;
            return result;
        }
        ++next_id;
        if (next_id == UINT32_MAX) {
            next_id = 1;
        }
    } while (next_id != start);
    return 0;
}

size_t CertStore::groupLimit(CertGroup group) const
{
    switch (group) {
        case CertGroup::V2GRoot:  return OCPP21_CERTSTORE_MAX_V2G_ROOT;
        case CertGroup::MORoot:   return OCPP21_CERTSTORE_MAX_MO_ROOT;
        case CertGroup::OEMRoot:  return OCPP21_CERTSTORE_MAX_OEM_ROOT;
        case CertGroup::CsmsRoot: return OCPP21_CERTSTORE_MAX_CSMS_ROOT;
        case CertGroup::MfrRoot:  return OCPP21_CERTSTORE_MAX_MFR_ROOT;
        default:                    return OCPP21_CERTSTORE_MAX_CHAINS;
    }
}

size_t CertStore::groupCount(CertGroup group) const
{
    size_t count = 0;
    for (auto &e : entries) {
        if (e.group == group) {
            ++count;
        }
    }
    return count;
}

const CertEntry *CertStore::find(CertGroup group) const
{
    for (auto &e : entries) {
        if (e.group == group) {
            return &e;
        }
    }
    return nullptr;
}

const CertEntry *CertStore::findById(uint32_t id) const
{
    for (auto &e : entries) {
        if (e.id == id) {
            return &e;
        }
    }
    return nullptr;
}

const CertEntry *CertStore::findSeccChainById(uint32_t id) const
{
    for (auto &e : entries) {
        if (e.id == id && (e.group == CertGroup::V2GChain || e.group == CertGroup::V2G20Chain)) {
            return &e;
        }
    }
    return nullptr;
}

std::string CertStore::pemPath(CertGroup group, uint32_t id) const
{
    char name[48];
    snprintf(name, sizeof(name), "/%s.%u.pem", group_prefixes[(size_t)group], id);
    return dir + name;
}

std::string CertStore::keyPath(uint32_t id) const
{
    char name[32];
    snprintf(name, sizeof(name), "/key.%u", id);
    return dir + name;
}

size_t CertStore::readPem(const CertEntry &e, char *buf, size_t buf_len) const
{
    if (buf == nullptr || buf_len == 0) {
        return 0;
    }
    std::string path = pemPath(e.group, e.id);
    size_t len = platform_read_file(path.c_str(), buf, buf_len - 1);
    buf[len] = '\0';
    return len;
}

bool CertStore::addEntry(CertGroup group, uint32_t id, const char *pem, bool require_anchor,
                         const OcppCertHashData21 *known_anchor, bool retain_replaced)
{
    CertEntry e;
    e.group = group;
    e.id = id;

    size_t cert_count = platform_cert_count21(pem);
    if (cert_count == 0) {
        return false;
    }

    OcppCertInfo21 info;
    if (!platform_cert_info21(pem, 0, &info)) {
        return false;
    }
    e.not_before = info.not_before;
    e.not_after = info.not_after;
    e.public_key_curve = info.public_key_curve;

    if (!is_chain_group(group)) {
        if (cert_count != 1 || !info.is_ca) {
            return false;
        }
        if (!platform_cert_hash_data21(pem, 0, nullptr, 0, &e.hash)) {
            return false;
        }
        entries.push_back(e);
        return true;
    }

    if (cert_count > (size_t)OCPP21_CHAIN_MAX_CHILDREN + 1) {
        return false;
    }

    // The anchor root provides the issuer key of the last chain
    // certificate and identifies the issuing PKI for A03 renewals.
    CertGroup root_group = group == CertGroup::CsmsClientChain ? CertGroup::CsmsRoot : CertGroup::V2GRoot;
    std::unique_ptr<char[]> root_bufs[OCPP21_CERTSTORE_MAX_OEM_ROOT];
    const char *root_ptrs[OCPP21_CERTSTORE_MAX_OEM_ROOT];
    size_t roots = loadRoots(root_group, root_bufs, root_ptrs, OCPP21_CERTSTORE_MAX_OEM_ROOT);
    size_t anchor_idx = 0;
    if (known_anchor != nullptr) {
        for (size_t i = 0; i < roots; ++i) {
            OcppCertHashData21 hash;
            if (platform_cert_hash_data21(root_ptrs[i], 0, nullptr, 0, &hash) && same_hash(hash, *known_anchor)) {
                anchor_idx = i;
                e.anchor_root = *known_anchor;
                e.has_anchor = true;
                break;
            }
        }
    } else if (roots > 0 && platform_verify_chain21(pem, root_ptrs, roots, e.not_before + 1, &anchor_idx) == OcppChainVerifyResult21::Ok) {
        e.has_anchor = platform_cert_hash_data21(root_ptrs[anchor_idx], 0, nullptr, 0, &e.anchor_root);
    }
    const char *anchor_pem = e.has_anchor ? root_ptrs[anchor_idx] : nullptr;

    for (size_t i = 0; i < cert_count; ++i) {
        const char *issuer_pem = pem;
        size_t issuer_idx = i + 1;
        if (i == cert_count - 1) {
            if (anchor_pem == nullptr) {
                break;
            }
            issuer_pem = anchor_pem;
            issuer_idx = 0;
        }
        auto *out = i == 0 ? &e.hash : &e.child_hash[i - 1];
        if (!platform_cert_hash_data21(pem, i, issuer_pem, issuer_idx, out)) {
            return false;
        }
        if (i > 0) {
            e.child_count = (uint8_t)i;
        }
    }

    if (!e.has_anchor) {
        if (require_anchor) {
            return false;
        }
        // CertificateSigned validates the chain before installation. Keeping
        // the unanchored logical copy preserves combined-certificate behavior
        // when a root is installed for only one of its two roles.
        log_warn("Certificate store: no anchor root for chain %s.%u", group_prefixes[(size_t)group], id);
    }

    for (const auto &existing : entries) {
        if (existing.group != group) {
            continue;
        }
        if (((group == CertGroup::CsmsClientChain) && !retain_replaced) ||
            ((group != CertGroup::CsmsClientChain) && existing.has_anchor &&
                e.has_anchor &&
                same_hash(existing.anchor_root, e.anchor_root) &&
                ((group != CertGroup::V2G20Chain) || (existing.public_key_curve == e.public_key_curve)))) {
            return false;
        }
    }

    entries.push_back(e);
    return true;
}

CertInstallResult CertStore::installRoot(CertGroup group, const char *pem, time_t now)
{
    if (is_chain_group(group) || pem == nullptr || strlen(pem) > OCPP21_ROOT_PEM_MAX) {
        return CertInstallResult::Rejected;
    }

    OcppCertInfo21 info;
    if (platform_cert_count21(pem) != 1 || !platform_cert_info21(pem, 0, &info) || !info.is_ca) {
        return CertInstallResult::Rejected;
    }

    // Not valid at time now, tolerating 300 s of clock skew for a notBefore in the near future (HUB20-42-001).
    if ((info.not_after < now) || (info.not_before > (now + 300))) {
        return CertInstallResult::Rejected;
    }

    OcppCertHashData21 hash;
    if (!platform_cert_hash_data21(pem, 0, nullptr, 0, &hash)) {
        return CertInstallResult::Rejected;
    }

    // M05.FR.17: replace an already installed certificate.
    for (auto &e : entries) {
        if (e.group != group || strcmp(e.hash.serial_number, hash.serial_number) != 0
         || strcmp(e.hash.issuer_name_hash, hash.issuer_name_hash) != 0
         || strcmp(e.hash.issuer_key_hash, hash.issuer_key_hash) != 0) {
            continue;
        }
        std::string path = pemPath(group, e.id);
        if (!platform_write_file(path.c_str(), (char *)pem, strlen(pem))) {
            return CertInstallResult::Failed;
        }
        e.not_before = info.not_before;
        e.not_after = info.not_after;
        return CertInstallResult::Accepted;
    }

    // M05.FR.06: reject when the storage limit would be exceeded.
    if (groupCount(group) >= groupLimit(group)) {
        return CertInstallResult::Rejected;
    }

    uint32_t id = nextId();
    if (id == 0) {
        return CertInstallResult::Failed;
    }
    std::string path = pemPath(group, id);
    if (!platform_write_file(path.c_str(), (char *)pem, strlen(pem))) {
        return CertInstallResult::Failed;
    }

    CertEntry e;
    e.group = group;
    e.id = id;
    e.hash = hash;
    e.not_before = info.not_before;
    e.not_after = info.not_after;
    entries.push_back(e);
    return CertInstallResult::Accepted;
}

ChainInstallResult CertStore::installChain(CertGroup group, uint32_t id, const char *pem,
                                           const OcppCertHashData21 &anchor_root, time_t now,
                                           bool combined, bool retain_replaced)
{
    if (!is_chain_group(group) || id == 0 || id == UINT32_MAX || pem == nullptr || strlen(pem) > OCPP21_CERT_PEM_MAX) {
        return ChainInstallResult::Failed;
    }

    const CertEntry *same_id_entry = findById(id);
    if (same_id_entry != nullptr && same_id_entry->group != group) {
        bool combined_pair = (group == CertGroup::CsmsClientChain && same_id_entry->group == CertGroup::V2GChain)
                          || (group == CertGroup::V2GChain && same_id_entry->group == CertGroup::CsmsClientChain);
        std::unique_ptr<char[]> existing;
        size_t existing_len = 0;
        if (!combined_pair
         || !read_pem_file(pemPath(same_id_entry->group, id), OCPP21_CERT_PEM_MAX, &existing, &existing_len)
         || existing_len != strlen(pem) || memcmp(existing.get(), pem, existing_len) != 0) {
            return ChainInstallResult::Failed;
        }
    }

    OcppCertInfo21 leaf_info;
    if (!platform_cert_info21(pem, 0, &leaf_info)) {
        return ChainInstallResult::Failed;
    }

    auto will_replace = [group, &anchor_root, &leaf_info](const CertEntry &e) {
        return e.group == group && (group == CertGroup::CsmsClientChain
            || (e.has_anchor && same_hash(e.anchor_root, anchor_root) &&
                (group != CertGroup::V2G20Chain || e.public_key_curve == leaf_info.public_key_curve)));
    };
    auto will_replace_final = [group, combined, &anchor_root, &leaf_info](const CertEntry &e) {
        if (!combined) {
            return e.group == group && (group == CertGroup::CsmsClientChain
                || (e.has_anchor && same_hash(e.anchor_root, anchor_root) &&
                    (group != CertGroup::V2G20Chain || e.public_key_curve == leaf_info.public_key_curve)));
        }
        return e.group == CertGroup::CsmsClientChain
            || (e.group == CertGroup::V2GChain && e.has_anchor && same_hash(e.anchor_root, anchor_root));
    };

    // A02.FR.15: delivery order must not let an older SECC certificate
    // displace the newest certificate from the same root and crypto suite.
    if (group == CertGroup::V2GChain || group == CertGroup::V2G20Chain) {
        for (const auto &e : entries) {
            if (will_replace(e) && e.not_before <= now && leaf_info.not_before <= now
             && e.not_before >= leaf_info.not_before) {
                return ChainInstallResult::RetainedExisting;
            }
        }
    }

    size_t resulting_credentials = chainCredentialCount();
    if (same_id_entry == nullptr) {
        ++resulting_credentials;
    }
    std::vector<uint32_t> removed_ids;
    for (const auto &e : entries) {
        if (!is_chain_group(e.group) || !will_replace_final(e) || e.id == id
         || std::find(removed_ids.begin(), removed_ids.end(), e.id) != removed_ids.end()) {
            continue;
        }
        bool id_remains = false;
        for (const auto &other : entries) {
            if (other.id == e.id && is_chain_group(other.group) && !will_replace_final(other)) {
                id_remains = true;
                break;
            }
        }
        if (!id_remains) {
            removed_ids.push_back(e.id);
            --resulting_credentials;
        }
    }
    if (resulting_credentials > OCPP21_CERTSTORE_MAX_CHAINS) {
        return ChainInstallResult::Failed;
    }
    reserveId(id);

    // A02.FR.15 / HUB20-42-002: the CSMS client chain is unique, SECC
    // chains are unique per anchoring root and crypto suite.
    for (size_t i = entries.size(); i > 0; --i) {
        auto &e = entries[i - 1];
        if (e.group != group) {
            continue;
        }
        if (!will_replace(e)) {
            continue;
        }
        if (retain_replaced && group == CertGroup::CsmsClientChain) {
            continue;
        }
        removeChain(e.group, e.id);
    }

    std::string path = pemPath(group, id);
    if (!platform_write_file(path.c_str(), (char *)pem, strlen(pem))) {
        return ChainInstallResult::Failed;
    }

    const bool require_anchor = !(combined && group == CertGroup::CsmsClientChain);
    if (!addEntry(group, id, pem, require_anchor, &anchor_root, retain_replaced)) {
        platform_remove_file(path.c_str());
        return ChainInstallResult::Failed;
    }
    return ChainInstallResult::Installed;
}

void CertStore::removeChain(CertGroup group, uint32_t id)
{
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].id != id || entries[i].group != group || !is_chain_group(entries[i].group)) {
            continue;
        }
        platform_remove_file(pemPath(group, id).c_str());
        entries.erase(entries.begin() + i);
        break;
    }

    // A combined certificate chain is installed under two groups sharing
    // one key file.
    for (auto &e : entries) {
        if (e.id == id && is_chain_group(e.group)) {
            return;
        }
    }
    platform_remove_file(keyPath(id).c_str());
}

CertDeleteResult CertStore::deleteByHash(const char *issuer_name_hash, const char *issuer_key_hash, const char *serial_number)
{
    // The hash data identifies the certificate, which may be installed
    // under multiple types. Delete every matching entry.
    bool deleted = false;
    bool in_use = false;

    for (size_t i = entries.size(); i > 0; --i) {
        auto &e = entries[i - 1];
        // HUB20-413-001: case-insensitive comparison.
        if (strcasecmp(e.hash.issuer_name_hash, issuer_name_hash) != 0
         || strcasecmp(e.hash.issuer_key_hash, issuer_key_hash) != 0
         || strcasecmp(e.hash.serial_number, serial_number) != 0) {
            continue;
        }

        if (e.group == CertGroup::CsmsClientChain) {
            in_use = true;
            continue;
        }

        if (is_chain_group(e.group)) {
            removeChain(e.group, e.id);
        } else {
            platform_remove_file(pemPath(e.group, e.id).c_str());
            entries.erase(entries.begin() + (i - 1));
        }
        deleted = true;
    }

    if (deleted) {
        return CertDeleteResult::Accepted;
    }
    return in_use ? CertDeleteResult::Failed : CertDeleteResult::NotFound;
}

size_t CertStore::loadRoots(CertGroup group, std::unique_ptr<char[]> *bufs, const char **ptrs, size_t max) const
{
    size_t count = 0;
    for (auto &e : entries) {
        if (e.group != group || count >= max) {
            continue;
        }
        size_t len = 0;
        if (!read_pem_file(pemPath(e.group, e.id), OCPP21_ROOT_PEM_MAX, &bufs[count], &len)) {
            continue;
        }
        ptrs[count] = bufs[count].get();
        ++count;
    }
    return count;
}

std::string CertStore::loadRootByHash(const OcppCertHashData21 &hash) const
{
    for (auto &e : entries) {
        if (is_chain_group(e.group) || strcmp(e.hash.issuer_key_hash, hash.issuer_key_hash) != 0
         || strcmp(e.hash.serial_number, hash.serial_number) != 0) {
            continue;
        }
        std::unique_ptr<char[]> buf;
        size_t len = 0;
        if (!read_pem_file(pemPath(e.group, e.id), OCPP21_ROOT_PEM_MAX, &buf, &len)) {
            return "";
        }
        return std::string(buf.get(), len);
    }
    return "";
}

} // namespace Ocpp21
