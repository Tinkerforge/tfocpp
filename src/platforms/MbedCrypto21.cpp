// mbedTLS implementation of the OCPP 2.1 certificate and key platform
// primitives (Platform21.h). Uses only mbedTLS and the platform file API.
// mbedTLS has no OCSP and no AIA support, both are implemented here on the
// ASN.1 layer per RFC 6960 and RFC 5280.
// Ed448 is not supported by mbedTLS, CSR generation for it fails.

#ifdef OCPP_CRYPTO_MBEDTLS

#define MBEDTLS_ALLOW_PRIVATE_ACCESS

#include <string.h>
#include <stdio.h>

#include <memory>

#include <mbedtls/asn1.h>
#include <mbedtls/base64.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecp.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>
#include <mbedtls/oid.h>
#include <mbedtls/pk.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>

#include "ocpp21/Platform21.h"
#include <common/Platform.h>

extern "C" time_t timegm(struct tm *tm);

struct CertList {
    mbedtls_x509_crt crt;
    bool ok = false;

    CertList(const char *pem)
    {
        mbedtls_x509_crt_init(&crt);
        if (pem != nullptr) {
            ok = mbedtls_x509_crt_parse(&crt, (const uint8_t *)pem, strlen(pem) + 1) == 0;
        }
    }

    ~CertList()
    {
        mbedtls_x509_crt_free(&crt);
    }

    CertList(const CertList &) = delete;
    CertList &operator=(const CertList &) = delete;

    mbedtls_x509_crt *at(size_t idx)
    {
        if (!ok) {
            return nullptr;
        }
        mbedtls_x509_crt *c = &crt;
        while (idx-- > 0) {
            c = c->next;
            if (c == nullptr) {
                return nullptr;
            }
        }
        return c;
    }

    size_t count()
    {
        if (!ok) {
            return 0;
        }
        size_t n = 0;
        for (mbedtls_x509_crt *c = &crt; c != nullptr; c = c->next) {
            ++n;
        }
        return n;
    }
};

struct Rng {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context drbg;
    bool ok;

    Rng()
    {
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&drbg);
        ok = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, nullptr, 0) == 0;
    }

    ~Rng()
    {
        mbedtls_ctr_drbg_free(&drbg);
        mbedtls_entropy_free(&entropy);
    }
};

static time_t x509_time_to_unix(const mbedtls_x509_time *t)
{
    struct tm tm_val = {};
    tm_val.tm_year = t->year - 1900;
    tm_val.tm_mon = t->mon - 1;
    tm_val.tm_mday = t->day;
    tm_val.tm_hour = t->hour;
    tm_val.tm_min = t->min;
    tm_val.tm_sec = t->sec;
    return timegm(&tm_val);
}

static bool md_hash(mbedtls_md_type_t type, const uint8_t *data, size_t len, uint8_t *out, size_t *out_len)
{
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(type);
    if (info == nullptr || mbedtls_md(info, data, len, out) != 0) {
        return false;
    }
    if (out_len != nullptr) {
        *out_len = mbedtls_md_get_size(info);
    }
    return true;
}

// Content of the subjectPublicKey bit string, without the unused bits
// octet (RFC 6960 CertID issuerKeyHash covers exactly this).
static bool spki_bitstring(const mbedtls_x509_buf *spki, mbedtls_asn1_buf *out)
{
    uint8_t *p = spki->p;
    const uint8_t *end = spki->p + spki->len;
    size_t len;
    if ((mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) ||
        (mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0)) {
        return false;
    }
    p += len;
    mbedtls_asn1_bitstring bs;
    if ((mbedtls_asn1_get_bitstring(&p, end, &bs) != 0) || (bs.unused_bits != 0)) {
        return false;
    }
    out->p = bs.p;
    out->len = bs.len;
    return true;
}

size_t platform_cert_count21(const char *pem)
{
    return CertList(pem).count();
}

bool platform_cert_info21(const char *pem, size_t idx, OcppCertInfo21 *info)
{
    CertList certs(pem);
    mbedtls_x509_crt *cert = certs.at(idx);
    if (cert == nullptr) {
        return false;
    }
    info->not_before = x509_time_to_unix(&cert->valid_from);
    info->not_after = x509_time_to_unix(&cert->valid_to);
    info->is_ca = cert->ca_istrue != 0;
    info->self_signed = (cert->issuer_raw.len == cert->subject_raw.len) && memcmp(cert->issuer_raw.p, cert->subject_raw.p, cert->issuer_raw.len) == 0;
    return true;
}

static void hex_encode(const uint8_t *data, size_t len, char *out)
{
    for (size_t i = 0; i < len; ++i) {
        sprintf(out + i * 2, "%02x", data[i]);
    }
}

bool platform_cert_hash_data21(const char *pem, size_t idx, const char *issuer_pem, size_t issuer_idx, OcppCertHashData21 *out)
{
    CertList certs(pem);
    mbedtls_x509_crt *cert = certs.at(idx);
    if (cert == nullptr) {
        return false;
    }
    CertList issuer_certs(issuer_pem != nullptr ? issuer_pem : pem);
    mbedtls_x509_crt *issuer = issuer_certs.at(issuer_pem != nullptr ? issuer_idx : idx);
    if (issuer == nullptr) {
        return false;
    }

    uint8_t hash[32];
    if (!md_hash(MBEDTLS_MD_SHA256, cert->issuer_raw.p, cert->issuer_raw.len, hash, nullptr)) {
        return false;
    }
    hex_encode(hash, sizeof(hash), out->issuer_name_hash);

    mbedtls_asn1_buf key;
    if (!spki_bitstring(&issuer->pk_raw, &key) || !md_hash(MBEDTLS_MD_SHA256, key.p, key.len, hash, nullptr)) {
        return false;
    }
    hex_encode(hash, sizeof(hash), out->issuer_key_hash);

    if (cert->serial.len == 0 || cert->serial.len * 2 >= sizeof(out->serial_number)) {
        return false;
    }
    size_t skip = 0;
    while (skip + 1 < cert->serial.len && cert->serial.p[skip] == 0) {
        ++skip;
    }
    hex_encode(cert->serial.p + skip, cert->serial.len - skip, out->serial_number);
    return true;
}

// Appends every parseable root to the list, unparseable ones are
// skipped like in the OpenSSL implementation.
static void build_root_store(mbedtls_x509_crt *store, const char * const *roots_pem, size_t roots_len)
{
    for (size_t i = 0; i < roots_len; ++i) {
        if (roots_pem[i] != nullptr) {
            mbedtls_x509_crt_parse(store, (const uint8_t *)roots_pem[i], strlen(roots_pem[i]) + 1);
        }
    }
}

// mbedTLS verifies validity against the wall clock without a check
// time parameter, so the time flags are masked here and validity is
// checked explicitly against now.
static OcppChainVerifyResult21 verify_against(mbedtls_x509_crt *chain, mbedtls_x509_crt *store, time_t now)
{
    uint32_t flags = 0;
    int ret = mbedtls_x509_crt_verify(chain, store, nullptr, nullptr, &flags, nullptr, nullptr);
    if (ret != 0 && ret != MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
        return OcppChainVerifyResult21::Invalid;
    }
    flags &= ~(uint32_t)(MBEDTLS_X509_BADCERT_EXPIRED | MBEDTLS_X509_BADCERT_FUTURE);
    if ((flags & MBEDTLS_X509_BADCERT_NOT_TRUSTED) != 0) {
        return OcppChainVerifyResult21::Untrusted;
    }
    if (flags != 0) {
        return OcppChainVerifyResult21::Invalid;
    }
    for (mbedtls_x509_crt *c = chain; c != nullptr; c = c->next) {
        if (x509_time_to_unix(&c->valid_from) > now) {
            return OcppChainVerifyResult21::NotYetValid;
        }
        if (x509_time_to_unix(&c->valid_to) < now) {
            return OcppChainVerifyResult21::Expired;
        }
    }
    return OcppChainVerifyResult21::Ok;
}

OcppChainVerifyResult21 platform_verify_chain21(const char *chain_pem, const char * const *roots_pem, size_t roots_len, time_t now, size_t *anchor_idx)
{
    CertList chain(chain_pem);
    if (!chain.ok) {
        return OcppChainVerifyResult21::Invalid;
    }

    mbedtls_x509_crt store;
    mbedtls_x509_crt_init(&store);
    build_root_store(&store, roots_pem, roots_len);
    OcppChainVerifyResult21 result = verify_against(&chain.crt, &store, now);
    mbedtls_x509_crt_free(&store);

    if (result == OcppChainVerifyResult21::Ok && anchor_idx != nullptr) {
        *anchor_idx = 0;
        for (size_t i = 0; i < roots_len; ++i) {
            mbedtls_x509_crt root;
            mbedtls_x509_crt_init(&root);
            build_root_store(&root, &roots_pem[i], 1);
            OcppChainVerifyResult21 single = verify_against(&chain.crt, &root, now);
            mbedtls_x509_crt_free(&root);
            if (single == OcppChainVerifyResult21::Ok) {
                *anchor_idx = i;
                break;
            }
        }
    }
    return result;
}

// Escapes the RFC 4514 special characters for the mbedTLS subject string parser.
static bool append_subject_part(char *subject, size_t subject_len, size_t *pos, const char *prefix, const char *value)
{
    for (const char *c = prefix; *c != '\0'; ++c) {
        if (*pos + 1 >= subject_len) {
            return false;
        }
        subject[(*pos)++] = *c;
    }
    for (const char *c = value; *c != '\0'; ++c) {
        if (strchr(",=+;\"\\<>#", *c) != nullptr) {
            if (*pos + 1 >= subject_len) {
                return false;
            }
            subject[(*pos)++] = '\\';
        }
        if (*pos + 1 >= subject_len) {
            return false;
        }
        subject[(*pos)++] = *c;
    }
    subject[*pos] = '\0';
    return true;
}

static size_t der_to_pem(const char *label, const uint8_t *der, size_t der_len, char *pem, size_t pem_len)
{
    uint8_t b64[3072];
    size_t b64_len = 0;
    if (mbedtls_base64_encode(b64, sizeof(b64), &b64_len, der, der_len) != 0) {
        return 0;
    }

    size_t need = strlen("-----BEGIN -----\n-----END -----\n") + 2 * strlen(label) + b64_len + (b64_len + 63) / 64 + 1;
    if (need > pem_len) {
        mbedtls_platform_zeroize(b64, sizeof(b64));
        return 0;
    }

    size_t pos = (size_t)snprintf(pem, pem_len, "-----BEGIN %s-----\n", label);
    for (size_t i = 0; i < b64_len; i += 64) {
        size_t chunk = b64_len - i < 64 ? b64_len - i : 64;
        memcpy(pem + pos, b64 + i, chunk);
        pos += chunk;
        pem[pos++] = '\n';
    }
    pos += (size_t)snprintf(pem + pos, pem_len - pos, "-----END %s-----\n", label);
    mbedtls_platform_zeroize(b64, sizeof(b64));
    return pos;
}

size_t platform_generate_csr21(const OcppCsrParams21 *params, char *csr_pem, size_t csr_pem_len)
{
    mbedtls_ecp_group_id group;
    mbedtls_md_type_t md;
    switch (params->curve) {
        case OcppCurve21::Secp256r1:
            group = MBEDTLS_ECP_DP_SECP256R1;
            md = MBEDTLS_MD_SHA256;
            break;
        case OcppCurve21::Secp521r1:
            group = MBEDTLS_ECP_DP_SECP521R1;
            md = MBEDTLS_MD_SHA512;
            break;
        case OcppCurve21::Ed448:
            return 0;
    }

    char subject[256];
    size_t pos = 0;
    if (!append_subject_part(subject, sizeof(subject), &pos, "CN=", params->common_name)) {
        return 0;
    }
    if ((params->organization != nullptr) && (params->organization[0] != '\0') && !append_subject_part(subject, sizeof(subject), &pos, ",O=", params->organization)) {
        return 0;
    }
    if ((params->country != nullptr) && (params->country[0] != '\0') && !append_subject_part(subject, sizeof(subject), &pos, ",C=", params->country)) {
        return 0;
    }

    Rng rng;
    if (!rng.ok) {
        return 0;
    }

    size_t result = 0;
    uint8_t key_pem[2048];
    mbedtls_pk_context key;
    mbedtls_pk_init(&key);
    mbedtls_x509write_csr req;
    mbedtls_x509write_csr_init(&req);

    if ((mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) == 0) &&
        (mbedtls_ecp_gen_key(group, mbedtls_pk_ec(key), mbedtls_ctr_drbg_random, &rng.drbg) == 0)) {
        mbedtls_x509write_csr_set_md_alg(&req, md);
        mbedtls_x509write_csr_set_key(&req, &key);
        if (mbedtls_x509write_csr_set_subject_name(&req, subject) == 0) {
            // The DER writers place the data at the end of the buffer.
            uint8_t der[2048];
            size_t csr_len = 0;
            int csr_der_len = mbedtls_x509write_csr_der(&req, der, sizeof(der), mbedtls_ctr_drbg_random, &rng.drbg);
            if (csr_der_len > 0) {
                csr_len = der_to_pem("CERTIFICATE REQUEST", der + sizeof(der) - (size_t)csr_der_len, (size_t)csr_der_len, csr_pem, csr_pem_len);
            }

            size_t key_len = 0;
            if (csr_len > 0) {
                int key_der_len = mbedtls_pk_write_key_der(&key, der, sizeof(der));
                if (key_der_len > 0) {
                    key_len = der_to_pem("EC PRIVATE KEY", der + sizeof(der) - (size_t)key_der_len, (size_t)key_der_len, (char *)key_pem, sizeof(key_pem));
                }
            }
            mbedtls_platform_zeroize(der, sizeof(der));

            if ((key_len > 0) && platform_write_file(params->key_name, (char *)key_pem, key_len)) {
                result = csr_len;
            }
        }
    }

    mbedtls_platform_zeroize(key_pem, sizeof(key_pem));
    mbedtls_x509write_csr_free(&req);
    mbedtls_pk_free(&key);
    if (result == 0) {
        platform_remove_file(params->key_name);
    }
    return result;
}

bool platform_key_matches_cert21(const char *key_name, const char *cert_pem)
{
    CertList certs(cert_pem);
    mbedtls_x509_crt *leaf = certs.at(0);
    if (leaf == nullptr) {
        return false;
    }

    char key_pem[4096];
    size_t key_len = platform_read_file(key_name, key_pem, sizeof(key_pem) - 1);
    if (key_len == 0) {
        return false;
    }
    key_pem[key_len] = '\0';

    Rng rng;
    bool result = false;
    mbedtls_pk_context key;
    mbedtls_pk_init(&key);
    if (rng.ok &&
        (mbedtls_pk_parse_key(&key, (const uint8_t *)key_pem, key_len + 1, nullptr, 0, mbedtls_ctr_drbg_random, &rng.drbg) == 0)) {
        result = mbedtls_pk_check_pair(&leaf->pk, &key, mbedtls_ctr_drbg_random, &rng.drbg) == 0;
    }
    mbedtls_pk_free(&key);
    mbedtls_platform_zeroize(key_pem, sizeof(key_pem));
    return result;
}

// OID content bytes.
static const uint8_t OID_AIA[] = {0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01};              // 1.3.6.1.5.5.7.1.1
static const uint8_t OID_AD_OCSP[] = {0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01};          // 1.3.6.1.5.5.7.48.1
static const uint8_t OID_OCSP_BASIC[] = {0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01}; // 1.3.6.1.5.5.7.48.1.1

static bool oid_equals(const mbedtls_asn1_buf *oid, const uint8_t *expected, size_t expected_len)
{
    return oid->len == expected_len && memcmp(oid->p, expected, expected_len) == 0;
}

bool platform_cert_ocsp_url21(const char *pem, size_t idx, char *url, size_t url_len)
{
    CertList certs(pem);
    mbedtls_x509_crt *cert = certs.at(idx);
    if ((cert == nullptr) || (cert->v3_ext.len == 0)) {
        return false;
    }

    uint8_t *p = cert->v3_ext.p;
    const uint8_t *end = p + cert->v3_ext.len;
    size_t len;
    if (mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
        return false;
    }
    while (p < end) {
        size_t ext_len;
        if (mbedtls_asn1_get_tag(&p, end, &ext_len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
            return false;
        }
        uint8_t *ext_end = p + ext_len;
        mbedtls_asn1_buf oid;
        if (mbedtls_asn1_get_tag(&p, ext_end, &oid.len, MBEDTLS_ASN1_OID) != 0) {
            return false;
        }
        oid.p = p;
        p += oid.len;
        if (!oid_equals(&oid, OID_AIA, sizeof(OID_AIA))) {
            p = ext_end;
            continue;
        }
        // Skip the optional critical flag.
        if (p < ext_end && *p == MBEDTLS_ASN1_BOOLEAN) {
            int critical;
            if (mbedtls_asn1_get_bool(&p, ext_end, &critical) != 0) {
                return false;
            }
        }
        size_t value_len;
        if (mbedtls_asn1_get_tag(&p, ext_end, &value_len, MBEDTLS_ASN1_OCTET_STRING) != 0) {
            return false;
        }
        const uint8_t *value_end = p + value_len;
        size_t aia_len;
        if (mbedtls_asn1_get_tag(&p, value_end, &aia_len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
            return false;
        }
        while (p < value_end) {
            size_t desc_len;
            if (mbedtls_asn1_get_tag(&p, value_end, &desc_len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
                return false;
            }
            uint8_t *desc_end = p + desc_len;
            mbedtls_asn1_buf method;
            if (mbedtls_asn1_get_tag(&p, desc_end, &method.len, MBEDTLS_ASN1_OID) != 0) {
                return false;
            }
            method.p = p;
            p += method.len;
            // GeneralName uniformResourceIdentifier is context tag 6.
            if (oid_equals(&method, OID_AD_OCSP, sizeof(OID_AD_OCSP))
             && p < desc_end && *p == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 6)) {
                size_t uri_len;
                ++p;
                if ((mbedtls_asn1_get_len(&p, desc_end, &uri_len) == 0) && (uri_len > 0) && (uri_len < url_len)) {
                    memcpy(url, p, uri_len);
                    url[uri_len] = '\0';
                    return true;
                }
                return false;
            }
            p = desc_end;
        }
        return false;
    }
    return false;
}

static size_t base64_decode(const char *in, uint8_t *out, size_t out_len)
{
    size_t in_len = strlen(in);
    auto clean = std::unique_ptr<uint8_t[]>(new uint8_t[in_len]);
    size_t clean_len = 0;
    for (size_t i = 0; i < in_len; ++i) {
        char c = in[i];
        if ((c != ' ') && (c != '\t') && (c != '\r') && (c != '\n')) {
            clean[clean_len++] = (uint8_t)c;
        }
    }
    size_t decoded = 0;
    if (mbedtls_base64_decode(out, out_len, &decoded, clean.get(), clean_len) != 0) {
        return 0;
    }
    return decoded;
}

static bool parse_generalized_time(const uint8_t *p, size_t len, time_t *out)
{
    // DER GeneralizedTime: YYYYMMDDHHMMSSZ
    if ((len != 15) || (p[14] != 'Z')) {
        return false;
    }
    unsigned digits[14];
    for (size_t i = 0; i < 14; ++i) {
        if ((p[i] < '0') || (p[i] > '9')) {
            return false;
        }
        digits[i] = (unsigned)(p[i] - '0');
    }
    struct tm tm_val = {};
    tm_val.tm_year = (int)(digits[0] * 1000 + digits[1] * 100 + digits[2] * 10 + digits[3]) - 1900;
    tm_val.tm_mon = (int)(digits[4] * 10 + digits[5]) - 1;
    tm_val.tm_mday = (int)(digits[6] * 10 + digits[7]);
    tm_val.tm_hour = (int)(digits[8] * 10 + digits[9]);
    tm_val.tm_min = (int)(digits[10] * 10 + digits[11]);
    tm_val.tm_sec = (int)(digits[12] * 10 + digits[13]);
    *out = timegm(&tm_val);
    return true;
}

struct OcspSingleResponse {
    mbedtls_asn1_buf hash_alg_oid;
    mbedtls_asn1_buf issuer_name_hash;
    mbedtls_asn1_buf issuer_key_hash;
    mbedtls_asn1_buf serial;
    uint8_t cert_status; // 0 good, 1 revoked, 2 unknown
    time_t this_update;
    time_t next_update; // 0 if absent
};

static bool parse_single_response(uint8_t **p, const uint8_t *end, OcspSingleResponse *out)
{
    size_t len;
    if (mbedtls_asn1_get_tag(p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
        return false;
    }
    uint8_t *single_end = *p + len;

    // CertID
    if (mbedtls_asn1_get_tag(p, single_end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
        return false;
    }
    uint8_t *certid_end = *p + len;
    mbedtls_asn1_buf alg_params;
    if ((mbedtls_asn1_get_alg(p, certid_end, &out->hash_alg_oid, &alg_params) != 0) ||
        (mbedtls_asn1_get_tag(p, certid_end, &out->issuer_name_hash.len, MBEDTLS_ASN1_OCTET_STRING) != 0)) {
        return false;
    }
    out->issuer_name_hash.p = *p;
    *p += out->issuer_name_hash.len;
    if (mbedtls_asn1_get_tag(p, certid_end, &out->issuer_key_hash.len, MBEDTLS_ASN1_OCTET_STRING) != 0) {
        return false;
    }
    out->issuer_key_hash.p = *p;
    *p += out->issuer_key_hash.len;
    if (mbedtls_asn1_get_tag(p, certid_end, &out->serial.len, MBEDTLS_ASN1_INTEGER) != 0) {
        return false;
    }
    out->serial.p = *p;
    *p = certid_end;

    // certStatus, IMPLICIT tags 0 (good), 1 (revoked, constructed), 2 (unknown)
    if (*p >= single_end) {
        return false;
    }
    uint8_t status_tag = **p;
    if (status_tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0)) {
        out->cert_status = 0;
    } else if (status_tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 1)) {
        out->cert_status = 1;
    } else if (status_tag == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | 2)) {
        out->cert_status = 2;
    } else {
        return false;
    }
    ++*p;
    if (mbedtls_asn1_get_len(p, single_end, &len) != 0) {
        return false;
    }
    *p += len;

    // thisUpdate
    if ((mbedtls_asn1_get_tag(p, single_end, &len, MBEDTLS_ASN1_GENERALIZED_TIME) != 0) ||
        !parse_generalized_time(*p, len, &out->this_update)) {
        return false;
    }
    *p += len;

    // nextUpdate [0] EXPLICIT, optional
    out->next_update = 0;
    if (*p < single_end && **p == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0)) {
        if ((mbedtls_asn1_get_tag(p, single_end, &len, MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0) != 0) ||
            (mbedtls_asn1_get_tag(p, single_end, &len, MBEDTLS_ASN1_GENERALIZED_TIME) != 0) ||
            !parse_generalized_time(*p, len, &out->next_update)) {
            return false;
        }
        *p += len;
    }

    // singleExtensions skipped
    *p = single_end;
    return true;
}

struct OcspBasicResponse {
    mbedtls_asn1_buf tbs;             // full tbsResponseData element, signed data
    mbedtls_asn1_buf responder_name;  // byName Name element, len 0 if byKey
    mbedtls_asn1_buf responder_key;   // byKey SHA1 key hash, len 0 if byName
    mbedtls_asn1_buf sig_alg_oid;
    mbedtls_asn1_buf signature;
    mbedtls_asn1_buf responses;       // content of the responses SEQUENCE
    mbedtls_x509_crt certs;           // embedded responder certificates
    bool has_certs;
};

static bool parse_basic_response(uint8_t *der, size_t der_len, OcspBasicResponse *out)
{
    uint8_t *p = der;
    const uint8_t *end = der + der_len;
    size_t len;

    // OCSPResponse
    if (mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
        return false;
    }
    end = p + len;
    int response_status;
    if (mbedtls_asn1_get_enum(&p, end, &response_status) != 0 || response_status != 0) {
        return false;
    }
    if ((mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0) != 0) ||
        (mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0)) {
        return false;
    }
    mbedtls_asn1_buf response_type;
    if (mbedtls_asn1_get_tag(&p, end, &response_type.len, MBEDTLS_ASN1_OID) != 0) {
        return false;
    }
    response_type.p = p;
    p += response_type.len;
    if (!oid_equals(&response_type, OID_OCSP_BASIC, sizeof(OID_OCSP_BASIC))) {
        return false;
    }
    if (mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OCTET_STRING) != 0) {
        return false;
    }
    end = p + len;

    // BasicOCSPResponse
    if (mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
        return false;
    }
    end = p + len;

    // tbsResponseData, keep the full element for the signature check
    out->tbs.p = p;
    if (mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
        return false;
    }
    uint8_t *tbs_end = p + len;
    out->tbs.len = (size_t)(tbs_end - out->tbs.p);

    // version [0] EXPLICIT, optional
    if (p < tbs_end && *p == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0)) {
        if (mbedtls_asn1_get_tag(&p, tbs_end, &len, MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0) != 0) {
            return false;
        }
        p += len;
    }

    // responderID CHOICE, EXPLICIT tags
    out->responder_name.len = 0;
    out->responder_key.len = 0;
    if (p < tbs_end && *p == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 1)) {
        if (mbedtls_asn1_get_tag(&p, tbs_end, &len, MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 1) != 0) {
            return false;
        }
        out->responder_name.p = p;
        out->responder_name.len = len;
        p += len;
    } else if (p < tbs_end && *p == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 2)) {
        if ((mbedtls_asn1_get_tag(&p, tbs_end, &len, MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 2) != 0) ||
            (mbedtls_asn1_get_tag(&p, tbs_end, &out->responder_key.len, MBEDTLS_ASN1_OCTET_STRING) != 0)) {
            return false;
        }
        out->responder_key.p = p;
        p += out->responder_key.len;
    } else {
        return false;
    }

    // producedAt
    if (mbedtls_asn1_get_tag(&p, tbs_end, &len, MBEDTLS_ASN1_GENERALIZED_TIME) != 0) {
        return false;
    }
    p += len;

    // responses
    if (mbedtls_asn1_get_tag(&p, tbs_end, &out->responses.len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
        return false;
    }
    out->responses.p = p;
    p = tbs_end; // responseExtensions skipped

    // signatureAlgorithm
    mbedtls_asn1_buf alg_params;
    if (mbedtls_asn1_get_alg(&p, end, &out->sig_alg_oid, &alg_params) != 0) {
        return false;
    }

    // signature, parsed manually since mbedtls_asn1_get_bitstring
    // requires the bit string to be the last element
    if ((mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_BIT_STRING) != 0) || (len < 2) || (*p != 0)) {
        return false;
    }
    out->signature.p = p + 1;
    out->signature.len = len - 1;
    p += len;

    // certs [0] EXPLICIT, optional
    out->has_certs = false;
    if ((p < end) && (*p == (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0))) {
        if (mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0) != 0) {
            return false;
        }
        const uint8_t *certs_end = p + len;
        if (mbedtls_asn1_get_tag(&p, certs_end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
            return false;
        }
        while (p < certs_end) {
            uint8_t *cert_start = p;
            size_t cert_len;
            if (mbedtls_asn1_get_tag(&p, certs_end, &cert_len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
                return false;
            }
            p += cert_len;
            if (mbedtls_x509_crt_parse_der(&out->certs, cert_start, (size_t)(p - cert_start)) != 0) {
                return false;
            }
            out->has_certs = true;
        }
    }
    return true;
}

static bool responder_matches(const OcspBasicResponse *basic, mbedtls_x509_crt *candidate)
{
    if (basic->responder_name.len != 0) {
        return (candidate->subject_raw.len == basic->responder_name.len) && (memcmp(candidate->subject_raw.p, basic->responder_name.p, basic->responder_name.len) == 0);
    }
    if (basic->responder_key.len != 0) {
        mbedtls_asn1_buf key;
        uint8_t hash[20];
        size_t hash_len;
        return spki_bitstring(&candidate->pk_raw, &key) &&
               md_hash(MBEDTLS_MD_SHA1, key.p, key.len, hash, &hash_len) &&
               (basic->responder_key.len == hash_len) &&
               (memcmp(basic->responder_key.p, hash, hash_len) == 0);
    }
    return false;
}

// The responder chain is verified against the given roots with the
// issuer available as untrusted intermediate (RFC 6960 4.2.2.2), a
// delegated responder additionally needs the OCSPSigning EKU.
static bool verify_responder(const OcspBasicResponse *basic, mbedtls_x509_crt *signer, mbedtls_x509_crt *issuer, bool delegated,
                             const char * const *roots_pem, size_t roots_len)
{
    if (delegated && mbedtls_x509_crt_check_extended_key_usage(signer, MBEDTLS_OID_OCSP_SIGNING, MBEDTLS_OID_SIZE(MBEDTLS_OID_OCSP_SIGNING)) != 0) {
        return false;
    }

    mbedtls_x509_crt chain;
    mbedtls_x509_crt_init(&chain);
    bool ok = mbedtls_x509_crt_parse_der(&chain, signer->raw.p, signer->raw.len) == 0;
    if (ok && basic->has_certs) {
        for (const mbedtls_x509_crt *c = &basic->certs; ok && c != nullptr; c = c->next) {
            if (c->raw.len == signer->raw.len && memcmp(c->raw.p, signer->raw.p, c->raw.len) == 0) {
                continue;
            }
            ok = mbedtls_x509_crt_parse_der(&chain, c->raw.p, c->raw.len) == 0;
        }
    }
    if (ok && (issuer->raw.len != signer->raw.len || memcmp(issuer->raw.p, signer->raw.p, issuer->raw.len) != 0)) {
        ok = mbedtls_x509_crt_parse_der(&chain, issuer->raw.p, issuer->raw.len) == 0;
    }

    if (ok) {
        mbedtls_x509_crt store;
        mbedtls_x509_crt_init(&store);
        build_root_store(&store, roots_pem, roots_len);
        uint32_t flags = 0;
        ok = mbedtls_x509_crt_verify(&chain, &store, nullptr, nullptr, &flags, nullptr, nullptr) == 0;
        mbedtls_x509_crt_free(&store);
    }
    mbedtls_x509_crt_free(&chain);
    return ok;
}

OcppOcspStatus21 platform_ocsp_validate21(const char *ocsp_response_b64,
                                          const char *pem, size_t idx,
                                          const char *issuer_pem, size_t issuer_idx,
                                          const char * const *roots_pem, size_t roots_len,
                                          time_t now, time_t *next_update)
{
    if (next_update != nullptr) {
        *next_update = 0;
    }

    CertList certs(pem);
    mbedtls_x509_crt *cert = certs.at(idx);
    CertList issuer_certs(issuer_pem);
    mbedtls_x509_crt *issuer = issuer_certs.at(issuer_idx);
    if (cert == nullptr || issuer == nullptr) {
        return OcppOcspStatus21::Invalid;
    }

    size_t der_max = strlen(ocsp_response_b64);
    auto der = std::unique_ptr<uint8_t[]>(new uint8_t[der_max + 3]);
    size_t der_len = base64_decode(ocsp_response_b64, der.get(), der_max + 3);
    if (der_len == 0) {
        return OcppOcspStatus21::Invalid;
    }

    OcspBasicResponse basic;
    mbedtls_x509_crt_init(&basic.certs);
    auto result = OcppOcspStatus21::Invalid;

    if (parse_basic_response(der.get(), der_len, &basic)) {
        // Find the signer, the issuer itself or an embedded responder.
        mbedtls_x509_crt *signer = nullptr;
        bool delegated = false;
        if (responder_matches(&basic, issuer)) {
            signer = issuer;
        } else if (basic.has_certs) {
            for (mbedtls_x509_crt *c = &basic.certs; c != nullptr; c = c->next) {
                if (responder_matches(&basic, c)) {
                    signer = c;
                    delegated = true;
                    break;
                }
            }
        }

        mbedtls_md_type_t sig_md;
        mbedtls_pk_type_t sig_pk;
        uint8_t hash[64];
        size_t hash_len;
        if ((signer != nullptr) &&
            (mbedtls_oid_get_sig_alg(&basic.sig_alg_oid, &sig_md, &sig_pk) == 0) &&
            (md_hash(sig_md, basic.tbs.p, basic.tbs.len, hash, &hash_len)) &&
            (mbedtls_pk_verify(&signer->pk, sig_md, hash, hash_len, basic.signature.p, basic.signature.len) == 0) &&
            verify_responder(&basic, signer, issuer, delegated, roots_pem, roots_len)) {
            mbedtls_asn1_buf issuer_key;
            uint8_t *p = basic.responses.p;
            const uint8_t *responses_end = p + basic.responses.len;
            OcspSingleResponse single;
            while (p < responses_end && spki_bitstring(&issuer->pk_raw, &issuer_key)) {
                if (!parse_single_response(&p, responses_end, &single)) {
                    break;
                }
                mbedtls_md_type_t certid_md;
                uint8_t name_hash[64], key_hash[64];
                size_t certid_hash_len;
                if ((mbedtls_oid_get_md_alg(&single.hash_alg_oid, &certid_md) != 0) ||
                    !md_hash(certid_md, issuer->subject_raw.p, issuer->subject_raw.len, name_hash, &certid_hash_len) ||
                    !md_hash(certid_md, issuer_key.p, issuer_key.len, key_hash, nullptr) ||
                    (single.issuer_name_hash.len != certid_hash_len) ||
                    (memcmp(single.issuer_name_hash.p, name_hash, certid_hash_len) != 0) ||
                    (single.issuer_key_hash.len != certid_hash_len) ||
                    (memcmp(single.issuer_key_hash.p, key_hash, certid_hash_len) != 0) ||
                    (single.serial.len != cert->serial.len) ||
                    (memcmp(single.serial.p, cert->serial.p, cert->serial.len) != 0)) {
                    continue;
                }
                // 300 s skew like OCSP_check_validity on the host.
                if ((single.this_update > now + 300) || ((single.next_update != 0) && (single.next_update < (now - 300)))) {
                    break;
                }
                switch (single.cert_status) {
                    case 0:  result = OcppOcspStatus21::Good; break;
                    case 1:  result = OcppOcspStatus21::Revoked; break;
                    default: result = OcppOcspStatus21::Unknown; break;
                }
                if (next_update != nullptr) {
                    *next_update = single.next_update;
                }
                break;
            }
        }
    }

    mbedtls_x509_crt_free(&basic.certs);
    return result;
}

#endif
