#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <mbedtls/platform_util.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_ticket.h>

#if defined(MBEDTLS_USE_PSA_CRYPTO)
#include <psa/crypto.h>
#endif

static int test_rng(void *ctx, unsigned char *output, size_t len)
{
    uint32_t *state = ctx;
    for (size_t i = 0; i < len; ++i) {
        *state = *state * 1664525u + 1013904223u;
        output[i] = (unsigned char)(*state >> 24);
    }
    return 0;
}

int main(void)
{
#if !defined(MBEDTLS_SSL_TICKET_C) || !defined(MBEDTLS_SSL_SESSION_TICKETS) || \
    !defined(MBEDTLS_SSL_PROTO_TLS1_3) || !defined(MBEDTLS_HAVE_TIME)
    return 77;
#else
    uint32_t rng_state = 1;
    mbedtls_ssl_ticket_context tickets;
    mbedtls_ssl_session source;
    mbedtls_ssl_session parsed;
    unsigned char ticket[1024];
    unsigned char ticket_copy[sizeof(ticket)];
    size_t ticket_len = 0;
    uint32_t lifetime = 0;

#if defined(MBEDTLS_USE_PSA_CRYPTO)
    assert(psa_crypto_init() == PSA_SUCCESS);
#endif
    mbedtls_ssl_ticket_init(&tickets);
    mbedtls_ssl_session_init(&source);
    mbedtls_ssl_session_init(&parsed);
    assert(mbedtls_ssl_ticket_setup(&tickets, test_rng, &rng_state,
                                    MBEDTLS_CIPHER_AES_256_GCM, 1) == 0);

    source.endpoint = MBEDTLS_SSL_IS_SERVER;
    source.tls_version = MBEDTLS_SSL_VERSION_TLS1_3;
    source.ciphersuite = MBEDTLS_TLS1_3_AES_128_GCM_SHA256;
    source.ticket_creation_time = mbedtls_ms_time();
    source.ticket_age_add = 0x12345678;
    source.ticket_flags = 0;
    source.resumption_key_len = 32;
    memset(source.resumption_key, 0x5a, source.resumption_key_len);

    assert(mbedtls_ssl_ticket_write(&tickets, &source, ticket,
                                    ticket + sizeof(ticket), &ticket_len,
                                    &lifetime) == 0);
    assert(lifetime == 1);
    memcpy(ticket_copy, ticket, ticket_len);
    assert(mbedtls_ssl_ticket_parse(&tickets, &parsed, ticket_copy, ticket_len) == 0);
    mbedtls_ssl_session_free(&parsed);
    mbedtls_ssl_session_init(&parsed);

    sleep(2);
    memcpy(ticket_copy, ticket, ticket_len);
    assert(mbedtls_ssl_ticket_parse(&tickets, &parsed, ticket_copy, ticket_len) ==
           MBEDTLS_ERR_SSL_SESSION_TICKET_EXPIRED);

    mbedtls_ssl_session_free(&parsed);
    mbedtls_ssl_session_free(&source);
    mbedtls_ssl_ticket_free(&tickets);
#if defined(MBEDTLS_USE_PSA_CRYPTO)
    mbedtls_psa_crypto_free();
#endif
    return 0;
#endif
}
