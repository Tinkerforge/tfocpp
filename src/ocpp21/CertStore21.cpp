#include "CertStore21.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/Platform.h>

namespace Ocpp21 {

static const char * const group_prefixes[] = {
    "v2gr", "mor", "oemr", "csmsr", "mfrr", "cs", "v2g2", "v2g20",
};

static bool is_chain_group(CertGroup group)
{
    return group == CertGroup::CsmsClientChain || group == CertGroup::V2GChain || group == CertGroup::V2G20Chain;
}

void CertStore::init(const char *charge_point_name)
{
    dir = std::string(charge_point_name) + ".certs";
    entries.clear();
    next_id = 1;

    struct Found {
        CertGroup group;
        uint32_t id;
    };
    std::vector<Found> found;

    void *d = platform_open_dir(dir.c_str());
    if (d != nullptr) {
        OcppDirEnt *ent;
        while ((ent = platform_read_dir(d)) != nullptr) {
            if (ent->is_dir) {
                continue;
            }
            char prefix[8] = "";
            uint32_t id = 0;
            if (sscanf(ent->name, "%7[a-z0-9].%u.pem", prefix, &id) != 2) {
                uint32_t key_id = 0;
                if (sscanf(ent->name, "key.%u", &key_id) == 1 && key_id >= next_id) {
                    next_id = key_id + 1;
                }
                continue;
            }
            for (size_t g = 0; g < sizeof(group_prefixes) / sizeof(group_prefixes[0]); ++g) {
                if (strcmp(prefix, group_prefixes[g]) == 0) {
                    found.push_back({(CertGroup)g, id});
                    if (id >= next_id) {
                        next_id = id + 1;
                    }
                    break;
                }
            }
        }
        platform_close_dir(d);
    }

    // Roots first, chains need them to recover the anchor root.
    auto buf = std::unique_ptr<char[]>(new char[OCPP21_CERT_PEM_MAX + 1]);
    for (int pass = 0; pass < 2; ++pass) {
        for (auto &f : found) {
            if (is_chain_group(f.group) != (pass == 1)) {
                continue;
            }
            std::string path = pemPath(f.group, f.id);
            size_t len = platform_read_file(path.c_str(), buf.get(), OCPP21_CERT_PEM_MAX);
            if (len == 0) {
                continue;
            }
            buf[len] = '\0';
            if (!addEntry(f.group, f.id, buf.get())) {
                log_warn("Certificate store: failed to load %s", path.c_str());
            }
        }
    }
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
    std::string path = pemPath(e.group, e.id);
    size_t len = platform_read_file(path.c_str(), buf, buf_len - 1);
    buf[len] = '\0';
    return len;
}

bool CertStore::addEntry(CertGroup group, uint32_t id, const char *pem)
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

    if (!is_chain_group(group)) {
        if (cert_count != 1) {
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
    if (roots > 0 && platform_verify_chain21(pem, root_ptrs, roots, e.not_before + 1, &anchor_idx) == OcppChainVerifyResult21::Ok) {
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
        // Without the anchor the issuer key hashes can not be computed.
        // Keep the chain usable for TLS but invisible to M03.
        log_warn("Certificate store: no anchor root for chain %s.%u", group_prefixes[(size_t)group], id);
    }

    entries.push_back(e);
    return true;
}

CertInstallResult CertStore::installRoot(CertGroup group, const char *pem, time_t now)
{
    if (is_chain_group(group)) {
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

bool CertStore::installChain(CertGroup group, uint32_t id, const char *pem, const OcppCertHashData21 &anchor_root)
{
    if (!is_chain_group(group)) {
        return false;
    }

    // A02.FR.13 / HUB20-42-002: the CSMS client chain is unique, SECC
    // chains are unique per anchoring root.
    for (size_t i = entries.size(); i > 0; --i) {
        auto &e = entries[i - 1];
        if (e.group != group) {
            continue;
        }
        if (group != CertGroup::CsmsClientChain
         && (!e.has_anchor || strcmp(e.anchor_root.issuer_key_hash, anchor_root.issuer_key_hash) != 0)) {
            continue;
        }
        removeChain(e.group, e.id);
    }

    std::string path = pemPath(group, id);
    if (!platform_write_file(path.c_str(), (char *)pem, strlen(pem))) {
        return false;
    }

    if (!addEntry(group, id, pem)) {
        platform_remove_file(path.c_str());
        return false;
    }
    return true;
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
        bufs[count] = std::unique_ptr<char[]>(new char[OCPP21_ROOT_PEM_MAX + 1]);
        size_t len = readPem(e, bufs[count].get(), OCPP21_ROOT_PEM_MAX + 1);
        if (len == 0) {
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
        auto buf = std::unique_ptr<char[]>(new char[OCPP21_ROOT_PEM_MAX + 1]);
        size_t len = readPem(e, buf.get(), OCPP21_ROOT_PEM_MAX + 1);
        if (len == 0) {
            return "";
        }
        return std::string(buf.get(), len);
    }
    return "";
}

} // namespace Ocpp21
