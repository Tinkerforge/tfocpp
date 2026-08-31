// OpenSSL implementation of the OCPP 2.1 certificate and key platform
// primitives (Platform21.h) for the Linux host.

#if defined(OCPP_PLATFORM_LINUX21) && !defined(OCPP_CRYPTO_MBEDTLS)

#include <string.h>
#include <sys/stat.h>

#include <memory>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/ocsp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "ocpp21/Platform21.h"
#include <common/Platform.h>

struct X509Deleter { void operator()(X509 *x) { X509_free(x); } };
using X509Ptr = std::unique_ptr<X509, X509Deleter>;

static X509Ptr load_cert(const char *pem, size_t idx)
{
    if (pem == nullptr) {
        return nullptr;
    }
    BIO *bio = BIO_new_mem_buf(pem, -1);
    if (bio == nullptr) {
        return nullptr;
    }
    X509 *cert = nullptr;
    for (size_t i = 0; i <= idx; ++i) {
        X509_free(cert);
        cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
        if (cert == nullptr) {
            break;
        }
    }
    BIO_free(bio);
    return X509Ptr{cert};
}

size_t platform_cert_count21(const char *pem)
{
    if (pem == nullptr) {
        return 0;
    }
    BIO *bio = BIO_new_mem_buf(pem, -1);
    if (bio == nullptr) {
        return 0;
    }
    size_t count = 0;
    while (true) {
        X509 *cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
        if (cert == nullptr) {
            break;
        }
        X509_free(cert);
        ++count;
    }
    BIO_free(bio);
    return count;
}

bool platform_cert_info21(const char *pem, size_t idx, OcppCertInfo21 *info)
{
    auto cert = load_cert(pem, idx);
    if (!cert) {
        return false;
    }

    struct tm tm_before = {}, tm_after = {};
    if (!ASN1_TIME_to_tm(X509_get0_notBefore(cert.get()), &tm_before)
     || !ASN1_TIME_to_tm(X509_get0_notAfter(cert.get()), &tm_after)) {
        return false;
    }

    info->not_before = timegm(&tm_before);
    info->not_after = timegm(&tm_after);
    info->is_ca = X509_check_ca(cert.get()) != 0;
    info->self_signed = X509_check_issued(cert.get(), cert.get()) == X509_V_OK;
    info->public_key_curve = OcppCurve21::Unknown;
    EVP_PKEY *key = X509_get0_pubkey(cert.get());
    if (key != nullptr && EVP_PKEY_is_a(key, "ED448")) {
        info->public_key_curve = OcppCurve21::Ed448;
    } else if (key != nullptr && EVP_PKEY_is_a(key, "EC")) {
        char group[32];
        size_t group_len = 0;
        if (EVP_PKEY_get_group_name(key, group, sizeof(group), &group_len) == 1) {
            if (strcmp(group, "prime256v1") == 0) {
                info->public_key_curve = OcppCurve21::Secp256r1;
            } else if (strcmp(group, "secp521r1") == 0) {
                info->public_key_curve = OcppCurve21::Secp521r1;
            }
        }
    }
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
    auto cert = load_cert(pem, idx);
    if (!cert) {
        return false;
    }
    auto issuer = issuer_pem != nullptr ? load_cert(issuer_pem, issuer_idx) : load_cert(pem, idx);
    if (!issuer) {
        return false;
    }

    uint8_t hash[SHA256_DIGEST_LENGTH];

    uint8_t *name_der = nullptr;
    int name_len = i2d_X509_NAME(X509_get_issuer_name(cert.get()), &name_der);
    if (name_len <= 0) {
        return false;
    }
    SHA256(name_der, (size_t)name_len, hash);
    OPENSSL_free(name_der);
    hex_encode(hash, sizeof(hash), out->issuer_name_hash);

    // issuerKeyHash covers the issuer public key bit string content, not
    // the full SPKI (RFC 6960 CertID).
    ASN1_BIT_STRING *key = X509_get0_pubkey_bitstr(issuer.get());
    if (key == nullptr) {
        return false;
    }
    SHA256(key->data, (size_t)key->length, hash);
    hex_encode(hash, sizeof(hash), out->issuer_key_hash);

    const ASN1_INTEGER *serial = X509_get0_serialNumber(cert.get());
    if (serial == nullptr || serial->length <= 0 || (size_t)serial->length * 2 >= sizeof(out->serial_number)) {
        return false;
    }
    size_t skip = 0;
    while (skip + 1 < (size_t)serial->length && serial->data[skip] == 0) {
        ++skip;
    }
    hex_encode(serial->data + skip, (size_t)serial->length - skip, out->serial_number);
    return true;
}

struct StackDeleter { void operator()(STACK_OF(X509) *s) { sk_X509_pop_free(s, X509_free); } };
using StackPtr = std::unique_ptr<STACK_OF(X509), StackDeleter>;

static StackPtr load_chain_rest(const char *chain_pem)
{
    StackPtr stack{sk_X509_new_null()};
    if (!stack) {
        return nullptr;
    }
    for (size_t i = 1;; ++i) {
        auto cert = load_cert(chain_pem, i);
        if (!cert) {
            break;
        }
        sk_X509_push(stack.get(), cert.release());
    }
    return stack;
}

static X509_STORE *build_root_store(const char * const *roots_pem, size_t roots_len)
{
    X509_STORE *store = X509_STORE_new();
    if (store == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < roots_len; ++i) {
        auto root = load_cert(roots_pem[i], 0);
        if (root) {
            X509_STORE_add_cert(store, root.get());
        }
    }
    return store;
}

OcppChainVerifyResult21 platform_verify_chain21(const char *chain_pem, const char * const *roots_pem, size_t roots_len, time_t now, size_t *anchor_idx)
{
    auto leaf = load_cert(chain_pem, 0);
    if (!leaf) {
        return OcppChainVerifyResult21::Invalid;
    }
    auto untrusted = load_chain_rest(chain_pem);
    X509_STORE *store = build_root_store(roots_pem, roots_len);
    X509_STORE_CTX *store_ctx = X509_STORE_CTX_new();
    if (store == nullptr || store_ctx == nullptr || !untrusted) {
        X509_STORE_CTX_free(store_ctx);
        X509_STORE_free(store);
        return OcppChainVerifyResult21::Invalid;
    }

    auto result = OcppChainVerifyResult21::Invalid;
    if (X509_STORE_CTX_init(store_ctx, store, leaf.get(), untrusted.get())) {
        X509_STORE_CTX_set_time(store_ctx, 0, now);
        X509_VERIFY_PARAM_clear_flags(X509_STORE_CTX_get0_param(store_ctx), X509_V_FLAG_USE_CHECK_TIME);
        X509_VERIFY_PARAM_set_flags(X509_STORE_CTX_get0_param(store_ctx), X509_V_FLAG_USE_CHECK_TIME);

        if (X509_verify_cert(store_ctx) == 1) {
            result = OcppChainVerifyResult21::Ok;
            if (anchor_idx != nullptr) {
                *anchor_idx = 0;
                STACK_OF(X509) *built = X509_STORE_CTX_get0_chain(store_ctx);
                X509 *anchor = sk_X509_value(built, sk_X509_num(built) - 1);
                for (size_t i = 0; i < roots_len; ++i) {
                    auto root = load_cert(roots_pem[i], 0);
                    if (root && X509_cmp(root.get(), anchor) == 0) {
                        *anchor_idx = i;
                        break;
                    }
                }
            }
        } else {
            switch (X509_STORE_CTX_get_error(store_ctx)) {
                case X509_V_ERR_CERT_NOT_YET_VALID:
                    result = OcppChainVerifyResult21::NotYetValid;
                    break;
                case X509_V_ERR_CERT_HAS_EXPIRED:
                    result = OcppChainVerifyResult21::Expired;
                    break;
                case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
                case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
                case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
                case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
                case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
                    result = OcppChainVerifyResult21::Untrusted;
                    break;
                default:
                    result = OcppChainVerifyResult21::Invalid;
                    break;
            }
        }
    }

    X509_STORE_CTX_free(store_ctx);
    X509_STORE_free(store);
    return result;
}

static BIO *open_key_file(const char *key_name)
{
    BIO *bio = BIO_new_file(key_name, "w");
    if (bio == nullptr) {
        const char *slash = strrchr(key_name, '/');
        if (slash != nullptr) {
            char dir[256];
            size_t dir_len = (size_t)(slash - key_name);
            if (dir_len < sizeof(dir)) {
                memcpy(dir, key_name, dir_len);
                dir[dir_len] = '\0';
                mkdir(dir, 0755);
                bio = BIO_new_file(key_name, "w");
            }
        }
    }
    if (bio != nullptr) {
        chmod(key_name, 0600);
    }
    return bio;
}

size_t platform_generate_csr21(const OcppCsrParams21 *params, char *csr_pem, size_t csr_pem_len)
{
    EVP_PKEY *key = nullptr;
    const EVP_MD *md = nullptr;

    switch (params->curve) {
        case OcppCurve21::Secp256r1:
            key = EVP_EC_gen("P-256");
            md = EVP_sha256();
            break;
        case OcppCurve21::Secp521r1:
            key = EVP_EC_gen("P-521");
            md = EVP_sha512();
            break;
        case OcppCurve21::Ed448:
            key = EVP_PKEY_Q_keygen(nullptr, nullptr, "ED448");
            md = nullptr;
            break;
        case OcppCurve21::Unknown:
            break;
    }
    if (key == nullptr) {
        return 0;
    }

    size_t result = 0;
    X509_REQ *req = X509_REQ_new();
    BIO *out = BIO_new(BIO_s_mem());
    BIO *key_out = open_key_file(params->key_name);

    if (req != nullptr && out != nullptr && key_out != nullptr) {
        X509_NAME *name = X509_REQ_get_subject_name(req);
        bool subject_ok = X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char *)params->common_name, -1, -1, 0) == 1;
        if (params->organization != nullptr && params->organization[0] != '\0') {
            subject_ok = subject_ok && X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (const unsigned char *)params->organization, -1, -1, 0) == 1;
        }
        if (params->country != nullptr && params->country[0] != '\0') {
            subject_ok = subject_ok && X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char *)params->country, -1, -1, 0) == 1;
        }
        if (params->domain_component != nullptr && params->domain_component[0] != '\0') {
            subject_ok = subject_ok && X509_NAME_add_entry_by_txt(name, "DC", MBSTRING_ASC, (const unsigned char *)params->domain_component, -1, -1, 0) == 1;
        }

        if (subject_ok
         && X509_REQ_set_pubkey(req, key) == 1
         && X509_REQ_sign(req, key, md) > 0
         && PEM_write_bio_X509_REQ(out, req) == 1
         && PEM_write_bio_PrivateKey(key_out, key, nullptr, nullptr, 0, nullptr, nullptr) == 1) {
            char *data = nullptr;
            long len = BIO_get_mem_data(out, &data);
            if (len > 0 && (size_t)len < csr_pem_len) {
                memcpy(csr_pem, data, (size_t)len);
                csr_pem[len] = '\0';
                result = (size_t)len;
            }
        }
    }

    BIO_free(key_out);
    BIO_free(out);
    X509_REQ_free(req);
    EVP_PKEY_free(key);
    if (result == 0) {
        platform_remove_file(params->key_name);
    }
    return result;
}

bool platform_key_matches_cert21(const char *key_name, const char *cert_pem)
{
    auto cert = load_cert(cert_pem, 0);
    if (!cert) {
        return false;
    }
    BIO *key_bio = BIO_new_file(key_name, "r");
    if (key_bio == nullptr) {
        return false;
    }
    EVP_PKEY *key = PEM_read_bio_PrivateKey(key_bio, nullptr, nullptr, nullptr);
    BIO_free(key_bio);
    if (key == nullptr) {
        return false;
    }
    bool result = X509_check_private_key(cert.get(), key) == 1;
    EVP_PKEY_free(key);
    return result;
}

static size_t base64_decode(const char *in, uint8_t *out, size_t out_len)
{
    EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
    if (ctx == nullptr) {
        return 0;
    }
    EVP_DecodeInit(ctx);
    int len = 0, final_len = 0;
    // EVP_DecodeUpdate skips whitespace and newlines.
    int rc = EVP_DecodeUpdate(ctx, out, &len, (const unsigned char *)in, (int)strlen(in));
    if (rc >= 0 && (size_t)len <= out_len) {
        rc = EVP_DecodeFinal(ctx, out + len, &final_len);
    }
    EVP_ENCODE_CTX_free(ctx);
    if (rc < 0 || (size_t)(len + final_len) > out_len) {
        return 0;
    }
    return (size_t)(len + final_len);
}

OcppOcspStatus21 platform_ocsp_validate21(const char *ocsp_response_b64,
                                          const char *pem, size_t idx,
                                          const char *issuer_pem, size_t issuer_idx,
                                          const char * const *roots_pem, size_t roots_len,
                                          time_t now, time_t *next_update,
                                          uint8_t *response_der, size_t response_der_cap,
                                          size_t *response_der_len,
                                          bool require_embedded_certs)
{
    (void)now;
    if (next_update != nullptr) {
        *next_update = 0;
    }
    if (response_der_len != nullptr) {
        *response_der_len = 0;
    }

    auto cert = load_cert(pem, idx);
    auto issuer = load_cert(issuer_pem, issuer_idx);
    if (!cert || !issuer) {
        return OcppOcspStatus21::Invalid;
    }

    size_t der_max = strlen(ocsp_response_b64);
    auto der = std::unique_ptr<uint8_t[]>(new uint8_t[der_max + 3]);
    size_t der_len = base64_decode(ocsp_response_b64, der.get(), der_max + 3);
    if (der_len == 0) {
        return OcppOcspStatus21::Invalid;
    }

    const uint8_t *p = der.get();
    OCSP_RESPONSE *resp = d2i_OCSP_RESPONSE(nullptr, &p, (long)der_len);
    if (resp == nullptr) {
        return OcppOcspStatus21::Invalid;
    }

    auto result = OcppOcspStatus21::Invalid;
    OCSP_BASICRESP *basic = nullptr;
    X509_STORE *store = nullptr;

    if (OCSP_response_status(resp) == OCSP_RESPONSE_STATUS_SUCCESSFUL
     && (basic = OCSP_response_get1_basic(resp)) != nullptr
     && (!require_embedded_certs || sk_X509_num(OCSP_resp_get0_certs(basic)) > 0)
     && (store = build_root_store(roots_pem, roots_len)) != nullptr) {
        // The issuer and the rest of its chain are needed as untrusted
        // intermediates to build the responder chain up to the root
        // (RFC 6960 4.2.2.2), the PKI may have more than one sub CA.
        StackPtr untrusted{sk_X509_new_null()};
        if (untrusted) {
            sk_X509_push(untrusted.get(), X509_dup(issuer.get()));
            for (size_t i = 0;; ++i) {
                if (i == issuer_idx) {
                    continue;
                }
                auto extra = load_cert(issuer_pem, i);
                if (!extra) {
                    break;
                }
                sk_X509_push(untrusted.get(), X509_dup(extra.get()));
            }
            if (OCSP_basic_verify(basic, untrusted.get(), store, 0) == 1) {
                // The CertID digest is chosen by the requester. Try SHA256
                // (A00.FR.506) first, then the SHA1 default.
                const EVP_MD *digests[] = {EVP_sha256(), EVP_sha1()};
                for (auto digest : digests) {
                    OCSP_CERTID *id = OCSP_cert_to_id(digest, cert.get(), issuer.get());
                    if (id == nullptr) {
                        continue;
                    }
                    int status = -1, reason = 0;
                    ASN1_GENERALIZEDTIME *revtime = nullptr, *thisupd = nullptr, *nextupd = nullptr;
                    if (OCSP_resp_find_status(basic, id, &status, &reason, &revtime, &thisupd, &nextupd) == 1) {
                        if (OCSP_check_validity(thisupd, nextupd, 300, -1) != 1) {
                            result = OcppOcspStatus21::Invalid;
                        } else {
                            switch (status) {
                                case V_OCSP_CERTSTATUS_GOOD:    result = OcppOcspStatus21::Good; break;
                                case V_OCSP_CERTSTATUS_REVOKED: result = OcppOcspStatus21::Revoked; break;
                                default:                        result = OcppOcspStatus21::Unknown; break;
                            }
                            if (next_update != nullptr && nextupd != nullptr) {
                                struct tm tm_next = {};
                                if (ASN1_TIME_to_tm(nextupd, &tm_next)) {
                                    *next_update = timegm(&tm_next);
                                }
                            }
                        }
                        OCSP_CERTID_free(id);
                        break;
                    }
                    OCSP_CERTID_free(id);
                }
            }
        }
    }

    X509_STORE_free(store);
    OCSP_BASICRESP_free(basic);
    OCSP_RESPONSE_free(resp);
    if (result == OcppOcspStatus21::Good && response_der != nullptr && response_der_len != nullptr && der_len <= response_der_cap) {
        memcpy(response_der, der.get(), der_len);
        *response_der_len = der_len;
    }
    return result;
}

bool platform_cert_ocsp_url21(const char *pem, size_t idx, char *url, size_t url_len)
{
    auto cert = load_cert(pem, idx);
    if (!cert) {
        return false;
    }
    STACK_OF(OPENSSL_STRING) *urls = X509_get1_ocsp(cert.get());
    if (urls == nullptr) {
        return false;
    }
    bool result = false;
    if (sk_OPENSSL_STRING_num(urls) > 0) {
        const char *first = sk_OPENSSL_STRING_value(urls, 0);
        if (first != nullptr && strlen(first) < url_len) {
            strcpy(url, first);
            result = true;
        }
    }
    X509_email_free(urls);
    return result;
}

#endif
