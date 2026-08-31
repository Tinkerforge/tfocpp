/* In-memory TLS 1.3 regression for Ed448 CertificateVerify. */
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ed448.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>

#include <stdio.h>
#include <string.h>

#define PIPE_CAPACITY 32768

struct byte_pipe {
    unsigned char data[PIPE_CAPACITY];
    size_t len;
};

struct endpoint_io {
    struct byte_pipe *incoming;
    struct byte_pipe *outgoing;
};

static int pipe_send(void *ctx, const unsigned char *buf, size_t len)
{
    struct endpoint_io *io = ctx;
    if (len > PIPE_CAPACITY - io->outgoing->len) {
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    }
    memcpy(io->outgoing->data + io->outgoing->len, buf, len);
    io->outgoing->len += len;
    return (int) len;
}

static int pipe_recv(void *ctx, unsigned char *buf, size_t len)
{
    struct endpoint_io *io = ctx;
    if (io->incoming->len == 0) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }
    if (len > io->incoming->len) {
        len = io->incoming->len;
    }
    memcpy(buf, io->incoming->data, len);
    memmove(io->incoming->data, io->incoming->data + len,
            io->incoming->len - len);
    io->incoming->len -= len;
    return (int) len;
}

static int is_retry(int ret)
{
    return ret == MBEDTLS_ERR_SSL_WANT_READ ||
           ret == MBEDTLS_ERR_SSL_WANT_WRITE;
}

static int make_certificate(mbedtls_pk_context *key, mbedtls_x509_crt *cert,
                            mbedtls_ctr_drbg_context *drbg)
{
    static const unsigned char seed[MBEDTLS_ED448_PRIVATE_KEY_SIZE] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
        0x39
    };
    unsigned char der[2048];
    unsigned char serial[] = { 1 };
    mbedtls_x509write_cert writer;
    int ret;

    mbedtls_x509write_crt_init(&writer);
    ret = mbedtls_pk_setup(key, mbedtls_pk_info_from_type(MBEDTLS_PK_ED448));
    if (ret == 0) {
        ret = mbedtls_ed448_import_private(mbedtls_pk_ed448(*key), seed,
                                            sizeof(seed));
    }
    if (ret == 0) {
        mbedtls_x509write_crt_set_subject_key(&writer, key);
        mbedtls_x509write_crt_set_issuer_key(&writer, key);
        mbedtls_x509write_crt_set_md_alg(&writer, MBEDTLS_MD_NONE);
        ret = mbedtls_x509write_crt_set_subject_name(&writer, "CN=localhost");
    }
    if (ret == 0) {
        ret = mbedtls_x509write_crt_set_issuer_name(&writer, "CN=localhost");
    }
    if (ret == 0) {
        ret = mbedtls_x509write_crt_set_serial_raw(&writer, serial,
                                                   sizeof(serial));
    }
    if (ret == 0) {
        ret = mbedtls_x509write_crt_set_validity(&writer, "20260101000000",
                                                 "20300101000000");
    }
    if (ret == 0) {
        ret = mbedtls_x509write_crt_set_basic_constraints(&writer, 0, -1);
    }
    if (ret == 0) {
        ret = mbedtls_x509write_crt_set_key_usage(
            &writer, MBEDTLS_X509_KU_DIGITAL_SIGNATURE);
    }
    if (ret == 0) {
        ret = mbedtls_x509write_crt_der(&writer, der, sizeof(der),
                                        mbedtls_ctr_drbg_random, drbg);
    }
    if (ret > 0) {
        ret = mbedtls_x509_crt_parse_der(cert, der + sizeof(der) - ret,
                                         (size_t) ret);
    }
    mbedtls_x509write_crt_free(&writer);
    return ret;
}

static int run_handshake(const uint16_t *client_sig_algs, int expect_success)
{
    static const uint16_t server_sig_algs[] = {
        MBEDTLS_TLS1_3_SIG_ED448,
        MBEDTLS_TLS1_3_SIG_ECDSA_SECP521R1_SHA512,
        MBEDTLS_TLS1_3_SIG_NONE
    };
    static const unsigned char personalization[] = "ed448 tls regression";
    struct byte_pipe client_to_server = {{0}, 0};
    struct byte_pipe server_to_client = {{0}, 0};
    struct endpoint_io client_io = {&server_to_client, &client_to_server};
    struct endpoint_io server_io = {&client_to_server, &server_to_client};
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context drbg;
    mbedtls_pk_context key;
    mbedtls_x509_crt cert;
    mbedtls_ssl_config client_conf;
    mbedtls_ssl_config server_conf;
    mbedtls_ssl_context client;
    mbedtls_ssl_context server;
    int client_ret = 0;
    int server_ret = 0;
    int selected = 0;
    int ret = 1;
    size_t i;

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&drbg);
    mbedtls_pk_init(&key);
    mbedtls_x509_crt_init(&cert);
    mbedtls_ssl_config_init(&client_conf);
    mbedtls_ssl_config_init(&server_conf);
    mbedtls_ssl_init(&client);
    mbedtls_ssl_init(&server);

    if (mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy,
                              personalization,
                              sizeof(personalization) - 1) != 0 ||
        make_certificate(&key, &cert, &drbg) != 0 ||
        mbedtls_ssl_config_defaults(&client_conf, MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT) != 0 ||
        mbedtls_ssl_config_defaults(&server_conf, MBEDTLS_SSL_IS_SERVER,
                                    MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
        goto cleanup;
    }

    mbedtls_ssl_conf_rng(&client_conf, mbedtls_ctr_drbg_random, &drbg);
    mbedtls_ssl_conf_rng(&server_conf, mbedtls_ctr_drbg_random, &drbg);
    mbedtls_ssl_conf_authmode(&client_conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_min_tls_version(&client_conf, MBEDTLS_SSL_VERSION_TLS1_3);
    mbedtls_ssl_conf_max_tls_version(&client_conf, MBEDTLS_SSL_VERSION_TLS1_3);
    mbedtls_ssl_conf_min_tls_version(&server_conf, MBEDTLS_SSL_VERSION_TLS1_3);
    mbedtls_ssl_conf_max_tls_version(&server_conf, MBEDTLS_SSL_VERSION_TLS1_3);
    mbedtls_ssl_conf_sig_algs(&client_conf, client_sig_algs);
    mbedtls_ssl_conf_sig_algs(&server_conf, server_sig_algs);
    if (mbedtls_ssl_conf_own_cert(&server_conf, &cert, &key) != 0 ||
        mbedtls_ssl_setup(&client, &client_conf) != 0 ||
        mbedtls_ssl_setup(&server, &server_conf) != 0) {
        goto cleanup;
    }
    mbedtls_ssl_set_bio(&client, &client_io, pipe_send, pipe_recv, NULL);
    mbedtls_ssl_set_bio(&server, &server_io, pipe_send, pipe_recv, NULL);

    for (i = 0; i < 10000; ++i) {
        if (client_ret == 0 || is_retry(client_ret)) {
            client_ret = mbedtls_ssl_handshake(&client);
        }
        if (server_ret == 0 || is_retry(server_ret)) {
            server_ret = mbedtls_ssl_handshake(&server);
        }
        if (mbedtls_ssl_get_hs_own_cert(&server) == &cert) {
            selected = 1;
        }
        if (client_ret == 0 && server_ret == 0) {
            break;
        }
        if ((!is_retry(client_ret) && client_ret != 0) ||
            (!is_retry(server_ret) && server_ret != 0)) {
            break;
        }
    }

    if (expect_success) {
        if (client_ret != 0 || server_ret != 0 || !selected) {
            fprintf(stderr, "Ed448 handshake failed: client=-0x%04x server=-0x%04x selected=%d\n",
                    -client_ret, -server_ret, selected);
            goto cleanup;
        }
    } else {
        if (client_ret == 0 && server_ret == 0) {
            fputs("Ed448-only server unexpectedly accepted a client without Ed448\n",
                  stderr);
            goto cleanup;
        }
        if (selected) {
            fputs("Ed448 certificate was selected although the client did not offer Ed448\n",
                  stderr);
            goto cleanup;
        }
    }
    ret = 0;

cleanup:
    mbedtls_ssl_free(&server);
    mbedtls_ssl_free(&client);
    mbedtls_ssl_config_free(&server_conf);
    mbedtls_ssl_config_free(&client_conf);
    mbedtls_x509_crt_free(&cert);
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&drbg);
    mbedtls_entropy_free(&entropy);
    return ret;
}

int main(void)
{
    static const uint16_t ed448_only[] = {
        MBEDTLS_TLS1_3_SIG_ED448,
        MBEDTLS_TLS1_3_SIG_NONE
    };
    static const uint16_t ecdsa_only[] = {
        MBEDTLS_TLS1_3_SIG_ECDSA_SECP521R1_SHA512,
        MBEDTLS_TLS1_3_SIG_NONE
    };

    if (run_handshake(ed448_only, 1) != 0 ||
        run_handshake(ecdsa_only, 0) != 0) {
        return 1;
    }
    puts("Ed448 TLS 1.3 handshake regression passed");
    return 0;
}
