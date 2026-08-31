#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

#include <common/Platform.h>
#include <ocpp21/CertStore21.h>

static std::map<std::string, std::string> files;
static std::vector<std::string> dir_entries;
static size_t dir_pos;
static OcppDirEnt dir_entry;

void platform_printfln(int, const char *, ...)
{
}

size_t platform_read_file(const char *name, char *buf, size_t len)
{
    auto found = files.find(name);
    if (found == files.end()) {
        return 0;
    }
    const size_t copied = found->second.size() < len ? found->second.size() : len;
    memcpy(buf, found->second.data(), copied);
    return copied;
}

bool platform_write_file(const char *name, char *buf, size_t len)
{
    files[name] = std::string(buf, len);
    return true;
}

void *platform_open_dir(const char *name)
{
    dir_entries.clear();
    dir_pos = 0;
    const std::string prefix = std::string(name) + "/";
    for (const auto &file : files) {
        if (file.first.compare(0, prefix.size(), prefix) == 0) {
            dir_entries.push_back(file.first.substr(prefix.size()));
        }
    }
    return dir_entries.empty() ? nullptr : &dir_entries;
}

OcppDirEnt *platform_read_dir(void *)
{
    if (dir_pos >= dir_entries.size()) {
        return nullptr;
    }
    dir_entry = {};
    snprintf(dir_entry.name, sizeof(dir_entry.name), "%s", dir_entries[dir_pos++].c_str());
    return &dir_entry;
}

void platform_close_dir(void *)
{
}

void platform_remove_file(const char *name)
{
    files.erase(name);
}

size_t platform_cert_count21(const char *pem)
{
    return strncmp(pem, "ROOT", 4) == 0 || strncmp(pem, "CHAIN", 5) == 0 ? 1 : 0;
}

bool platform_cert_info21(const char *pem, size_t idx, OcppCertInfo21 *info)
{
    if (idx != 0 || platform_cert_count21(pem) == 0) {
        return false;
    }
    const OcppCurve21 curve = strstr(pem, "ED448") != nullptr ? OcppCurve21::Ed448 : OcppCurve21::Secp521r1;
    *info = {1, 4102444800, strncmp(pem, "ROOT", 4) == 0, strncmp(pem, "ROOT", 4) == 0, curve};
    return true;
}

bool platform_cert_hash_data21(const char *pem, size_t, const char *, size_t, OcppCertHashData21 *out)
{
    memset(out, 0, sizeof(*out));
    snprintf(out->issuer_name_hash, sizeof(out->issuer_name_hash), "%064u", 1u);
    snprintf(out->issuer_key_hash, sizeof(out->issuer_key_hash), "%064u", 2u);
    snprintf(out->serial_number, sizeof(out->serial_number), "%s", pem);
    return true;
}

OcppChainVerifyResult21 platform_verify_chain21(const char *chain, const char * const *roots, size_t roots_len, time_t, size_t *anchor_idx)
{
    if (roots_len == 0 || strncmp(chain, "CHAIN", 5) != 0) {
        return OcppChainVerifyResult21::Untrusted;
    }
    const size_t chain_suffix_len = strcspn(chain + 5, ":");
    for (size_t i = 0; i < roots_len; ++i) {
        if (strlen(roots[i] + 4) != chain_suffix_len || strncmp(chain + 5, roots[i] + 4, chain_suffix_len) != 0) {
            continue;
        }
        if (anchor_idx != nullptr) {
            *anchor_idx = i;
        }
        return OcppChainVerifyResult21::Ok;
    }
    return OcppChainVerifyResult21::Untrusted;
}

bool platform_key_matches_cert21(const char *key_name, const char *cert_pem)
{
    auto key = files.find(key_name);
    return key != files.end() && key->second == cert_pem;
}

int main()
{
    using namespace Ocpp21;

    files["station.certs/v2gr.1.pem"] = "ROOT-2";
    files["station.certs/v2g2.2.pem"] = "CHAIN-2";
    files["station.certs/key.2"] = "CHAIN-2";
    files["station.certs/v2g20.3.pem"] = "CHAIN-3";
    files["station.certs/key.3"] = "wrong";
    files["station.certs/key.4"] = "orphan";
    files["station.certs/v2gr.5.pem.tmp"] = "ROOT-5";

    CertStore recovered;
    recovered.init("station");
    assert(recovered.count() == 2);
    assert(recovered.findSeccChainById(2) != nullptr);
    assert(recovered.findSeccChainById(3) == nullptr);
    assert(files.count("station.certs/key.3") == 1);
    assert(files.count("station.certs/key.4") == 0);
    assert(recovered.nextId() == 4);

    files.clear();
    files["combined.certs/v2gr.1.pem"] = "ROOT-combined";
    files["combined.certs/cs.2.pem"] = "CHAIN-combined";
    files["combined.certs/v2g2.2.pem"] = "CHAIN-combined";
    files["combined.certs/key.2"] = "CHAIN-combined";
    CertStore combined;
    combined.init("combined");
    assert(combined.count() == 2);
    assert(combined.find(CertGroup::CsmsClientChain) != nullptr);
    assert(combined.find(CertGroup::V2GChain) != nullptr);

    files.clear();
    files["csms-only.certs/csmsr.1.pem"] = "ROOT-combined";
    files["csms-only.certs/cs.2.pem"] = "CHAIN-combined";
    files["csms-only.certs/v2g2.2.pem"] = "CHAIN-combined";
    files["csms-only.certs/key.2"] = "CHAIN-combined";
    CertStore csms_only;
    csms_only.init("csms-only");
    assert(csms_only.find(CertGroup::CsmsClientChain) != nullptr);
    assert(csms_only.find(CertGroup::V2GChain) == nullptr);

    files.clear();
    for (uint32_t id = 10; id < 15; ++id) {
        const std::string chain = "CHAIN-" + std::to_string(id);
        files["limit.certs/v2gr." + std::to_string(id - 9) + ".pem"] = "ROOT-" + std::to_string(id);
        files["limit.certs/v2g2." + std::to_string(id) + ".pem"] = chain;
        files["limit.certs/key." + std::to_string(id)] = chain;
    }
    CertStore limited;
    limited.init("limit");
    assert(limited.count() == 5 + OCPP21_CERTSTORE_MAX_CHAINS);
    assert(limited.findSeccChainById(14) != nullptr);
    assert(limited.findSeccChainById(10) == nullptr);

    files.clear();
    files["dual.certs/v2gr.1.pem"] = "ROOT-dual";
    files["dual.certs/key.2"] = "CHAIN-dual";
    CertStore dual;
    dual.init("dual");
    OcppCertHashData21 anchor{};
    assert(platform_cert_hash_data21("ROOT-dual", 0, nullptr, 0, &anchor));
    assert(dual.installChain(CertGroup::V2G20Chain, 2, "CHAIN-dual", anchor));
    files["dual.certs/key.3"] = "CHAIN-dual:ED448";
    assert(dual.installChain(CertGroup::V2G20Chain, 3, "CHAIN-dual:ED448", anchor));
    assert(dual.findSeccChainById(2) != nullptr);
    assert(dual.findSeccChainById(3) != nullptr);
    files["dual.certs/key.4"] = "CHAIN-dual:ED448-renewed";
    assert(dual.installChain(CertGroup::V2G20Chain, 4, "CHAIN-dual:ED448-renewed", anchor));
    assert(dual.findSeccChainById(2) != nullptr);
    assert(dual.findSeccChainById(3) == nullptr);
    assert(dual.findSeccChainById(4) != nullptr);

    std::string large_root = "ROOT-large";
    large_root.resize(5000, '\n');
    assert(limited.installRoot(CertGroup::CsmsRoot, large_root.c_str(), 100) == CertInstallResult::Accepted);
    std::unique_ptr<char[]> roots[OCPP21_CERTSTORE_MAX_CSMS_ROOT];
    const char *root_ptrs[OCPP21_CERTSTORE_MAX_CSMS_ROOT];
    assert(limited.loadRoots(CertGroup::CsmsRoot, roots, root_ptrs, OCPP21_CERTSTORE_MAX_CSMS_ROOT) == 1);
    assert(strlen(root_ptrs[0]) == large_root.size());

    std::string maximum_root = "ROOT-maximum";
    maximum_root.resize(OCPP21_ROOT_PEM_MAX, '\n');
    assert(limited.installRoot(CertGroup::MfrRoot, maximum_root.c_str(), 100) == CertInstallResult::Accepted);
    maximum_root.push_back('\n');
    assert(limited.installRoot(CertGroup::MORoot, maximum_root.c_str(), 100) == CertInstallResult::Rejected);

    return 0;
}
