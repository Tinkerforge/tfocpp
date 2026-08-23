#if defined(OCPP_PLATFORM_LINUX) || defined(OCPP_PLATFORM_TEST) || defined(OCPP_PLATFORM_LINUX21)

#include "LinuxWS.h"

#include <common/Platform.h>
#include <common/Tools.h>

#define URL_PARSER_IMPLEMENTATION
#include "lib/url.h"

#include "mongoose.h"

#if MG_ENABLE_OPENSSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/sslerr.h>
#include <openssl/x509_vfy.h>
#endif

#include <memory>
#include <string>
#include <vector>

// TLS 1.2 cipher suites per OCPP 2.1 security profiles 2/3: the required
// ECDHE_ECDSA and RSA AES GCM pairs plus ECDHE_RSA for forward secrecy with
// RSA certificates. TLS 1.3 suites are configured by OpenSSL defaults.
#define OCPP_TLS_CIPHERS "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:AES128-GCM-SHA256:AES256-GCM-SHA384"

// The mongoose event manager is shared by all websocket contexts.
static struct mg_mgr mgr;
static size_t ctx_count = 0;

struct LinuxWsContext {
    struct mg_connection *conn = nullptr;
    std::unique_ptr<char[]> url;
    std::string subprotocol;
    std::string tls_ca_file;
    std::string tls_client_cert_file;
    std::string tls_client_key_file;
    std::vector<std::unique_ptr<char[]>> auth_header_bufs;
    size_t next_auth_header = 0;
    bool connected = false;
    bool is_ssl = false;

    void (*recv_cb)(char *, size_t, void *) = nullptr;
    void *recv_cb_userdata = nullptr;

    void (*pong_cb)(void *) = nullptr;
    void *pong_cb_userdata = nullptr;

    void (*conn_error_cb)(PlatformConnectionError, void *) = nullptr;
    void *conn_error_cb_userdata = nullptr;
};

#if MG_ENABLE_OPENSSL
static PlatformConnectionError classify_tls_error() {
    unsigned long openssl_error = 0;
    long verify_result = X509_V_OK;
    mg_tls_get_last_handshake_error(&openssl_error, &verify_result);

    if (verify_result != X509_V_OK)
        return PlatformConnectionError::InvalidCsmsCertificate;

    switch (ERR_GET_REASON(openssl_error)) {
        case SSL_R_CERTIFICATE_VERIFY_FAILED:
            return PlatformConnectionError::InvalidCsmsCertificate;
        case SSL_R_UNSUPPORTED_PROTOCOL:
        case SSL_R_VERSION_TOO_LOW:
        case SSL_R_TLSV1_ALERT_PROTOCOL_VERSION:
            return PlatformConnectionError::InvalidTlsVersion;
        case SSL_R_NO_CIPHERS_AVAILABLE:
        case SSL_R_NO_SHARED_CIPHER:
        case SSL_R_SSLV3_ALERT_HANDSHAKE_FAILURE:
        case SSL_R_TLSV1_ALERT_INSUFFICIENT_SECURITY:
            return PlatformConnectionError::InvalidTlsCipherSuite;
        default:
            return PlatformConnectionError::Unknown;
    }
}
#endif

static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    LinuxWsContext *ctx = (LinuxWsContext *)fn_data;

    if (ev == MG_EV_CONNECT) {
        if (ctx->is_ssl) {
            struct mg_tls_opts opts = {};
            opts.ca = ctx->tls_ca_file.c_str();
            if (!ctx->tls_client_cert_file.empty()) {
                opts.cert = ctx->tls_client_cert_file.c_str();
                opts.certkey = ctx->tls_client_key_file.c_str();
            }
            opts.ciphers = OCPP_TLS_CIPHERS;
            // Enables hostname verification against the certificate.
            opts.srvname = mg_url_host(ctx->url.get());
#if MG_ENABLE_OPENSSL
            mg_tls_clear_last_handshake_error();
#endif
            mg_tls_init(c, &opts);
        }
    } else if (ev == MG_EV_ERROR) {
        MG_ERROR(("%p %s", c->fd, (char *) ev_data));
#if MG_ENABLE_OPENSSL
        if (ctx->is_ssl && ctx->conn_error_cb != nullptr) {
            ctx->conn_error_cb(classify_tls_error(), ctx->conn_error_cb_userdata);
            mg_tls_clear_last_handshake_error();
        }
#endif
    } else if (ev == MG_EV_WS_OPEN) {
        ctx->connected = true;
    } else if (ev == MG_EV_WS_MSG) {
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;

        auto useless_copy = std::unique_ptr<char[]>(new char[wm->data.len]);
        memcpy(useless_copy.get(), wm->data.ptr, wm->data.len);
        if (ctx->recv_cb != nullptr) {
            ctx->recv_cb(useless_copy.get(), wm->data.len, ctx->recv_cb_userdata);
        }
    } else if (ev == MG_EV_WS_CTL) {
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        uint8_t op = wm->flags & 15;
        if (op == WEBSOCKET_OP_PONG && ctx->pong_cb != nullptr)
            ctx->pong_cb(ctx->pong_cb_userdata);
    }

    if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE) {
        ctx->connected = false;
        ctx->conn = nullptr;
    }
}

static void platform_connect(LinuxWsContext *ctx) {
    char headers[512];
    if (ctx->auth_header_bufs.size() > 0) {
        snprintf(headers, sizeof(headers), "Sec-WebSocket-Protocol: %s\r\nAuthorization: Basic %s\r\n", ctx->subprotocol.c_str(), ctx->auth_header_bufs[ctx->next_auth_header].get());
        ctx->next_auth_header = (ctx->next_auth_header + 1) % ctx->auth_header_bufs.size();
    } else {
        snprintf(headers, sizeof(headers), "Sec-WebSocket-Protocol: %s\r\n", ctx->subprotocol.c_str());
    }
    ctx->conn = mg_ws_connect(&mgr, ctx->url.get(), fn, ctx, "%s", headers);
}

static void build_auth_headers(LinuxWsContext *ctx, BasicAuthCredentials *credentials, size_t credentials_length)
{
    ctx->auth_header_bufs.clear();
    ctx->next_auth_header = 0;

    if (credentials == nullptr)
        return;

    for (size_t cred_idx = 0; cred_idx < credentials_length; ++cred_idx) {
        auto auth_header_buf = heap_alloc_array<char>(2 * (strlen(credentials[cred_idx].user.get()) + credentials[cred_idx].pass_length + 1) + 1);

        int offset = 0;
        for(size_t i = 0; i < strlen(credentials[cred_idx].user.get()); ++i)
            offset = mg_base64_update(credentials[cred_idx].user[i], auth_header_buf.get(), offset);

        offset = mg_base64_update(':', auth_header_buf.get(), offset);

        for(size_t i = 0; i < credentials[cred_idx].pass_length; ++i)
            offset = mg_base64_update(credentials[cred_idx].pass[i], auth_header_buf.get(), offset);

        offset = mg_base64_final(auth_header_buf.get(), offset);
        ctx->auth_header_bufs.push_back(std::move(auth_header_buf));
    }
}

void *platform_init(const char *websocket_url, const char *subprotocol, BasicAuthCredentials *credentials, size_t credentials_length, const PlatformTlsConfig *tls)
{
    bool is_ssl = mg_url_is_ssl(websocket_url);

    if (is_ssl && (tls == nullptr || tls->ca_cert_file == nullptr)) {
        log_error("A wss:// URL requires a CA certificate for CSMS certificate verification");
        return nullptr;
    }

    if (ctx_count == 0)
        mg_mgr_init(&mgr);
    ++ctx_count;

    auto ctx = new LinuxWsContext();

    ctx->is_ssl = is_ssl;
    ctx->subprotocol = subprotocol;
    if (tls != nullptr) {
        if (tls->ca_cert_file != nullptr)
            ctx->tls_ca_file = tls->ca_cert_file;
        if (tls->client_cert_file != nullptr)
            ctx->tls_client_cert_file = tls->client_cert_file;
        if (tls->client_key_file != nullptr)
            ctx->tls_client_key_file = tls->client_key_file;
    }

    auto url_len = strlen(websocket_url);
    ctx->url = heap_alloc_array<char>(url_len + 1);
    memcpy(ctx->url.get(), websocket_url, url_len + 1); // copy with null-terminator

    build_auth_headers(ctx, credentials, credentials_length);

    platform_connect(ctx);

    return ctx;
}

void platform_update_credentials(void *_ctx, BasicAuthCredentials *credentials, size_t credentials_length)
{
    LinuxWsContext *ctx = (LinuxWsContext *)_ctx;

    build_auth_headers(ctx, credentials, credentials_length);
}

void platform_update_tls(void *_ctx, const PlatformTlsConfig *tls)
{
    LinuxWsContext *ctx = (LinuxWsContext *)_ctx;

    ctx->tls_ca_file = tls != nullptr && tls->ca_cert_file != nullptr ? tls->ca_cert_file : "";
    ctx->tls_client_cert_file = tls != nullptr && tls->client_cert_file != nullptr ? tls->client_cert_file : "";
    ctx->tls_client_key_file = tls != nullptr && tls->client_key_file != nullptr ? tls->client_key_file : "";
}

void platform_disconnect(void *_ctx) {
    LinuxWsContext *ctx = (LinuxWsContext *)_ctx;

    if (!ctx->connected || ctx->conn == nullptr)
        return;

    mg_ws_send(ctx->conn, "", 0, WEBSOCKET_OP_CLOSE);
    ctx->conn->is_draining = 1;

    while(ctx->connected)
        mg_mgr_poll(&mgr, 1);
}

void platform_reconnect(void *_ctx) {
    LinuxWsContext *ctx = (LinuxWsContext *)_ctx;

    platform_disconnect(ctx);
    platform_connect(ctx);
}

void platform_destroy(void *_ctx) {
    LinuxWsContext *ctx = (LinuxWsContext *)_ctx;

    platform_disconnect(ctx);
    if (ctx->conn != nullptr) {
        ctx->conn->is_closing = 1;
        while (ctx->conn != nullptr)
            mg_mgr_poll(&mgr, 1);
    }

    delete ctx;

    --ctx_count;
    if (ctx_count == 0)
        mg_mgr_free(&mgr);
}

bool platform_ws_connected(void *_ctx)
{
    return ((LinuxWsContext *)_ctx)->connected;
}

bool platform_ws_send(void *_ctx, const char *buf, size_t buf_len)
{
    LinuxWsContext *ctx = (LinuxWsContext *)_ctx;

    if (ctx->conn == nullptr)
        return false;

    // mg_ws_send returns the sent bytes including the (variable length) web socket frame header.
    // Checking for success with mg_ws_send() == buf_len is thus not possible.
    // As the ws header length is unknown, we can't check for == buf_len + [hardcoded] header len
    // -> Detecting a short write is not possible.
    // However ms_ws_send internally does not check for short writes and just reurns buf_len + header_len.
    // Assume that sending data always succeeds.
    mg_ws_send(ctx->conn, buf, buf_len, WEBSOCKET_OP_TEXT);
    return true;
}

bool platform_ws_send_ping(void *_ctx) {
    LinuxWsContext *ctx = (LinuxWsContext *)_ctx;

    if (ctx->conn == nullptr)
        return false;

    mg_ws_send(ctx->conn, "", 0, WEBSOCKET_OP_PING);
    return true;
}

void platform_ws_register_receive_callback(void *_ctx, void(*cb)(char *, size_t, void *), void *user_data)
{
    LinuxWsContext *ctx = (LinuxWsContext *)_ctx;

    ctx->recv_cb = cb;
    ctx->recv_cb_userdata = user_data;
}

void platform_ws_register_pong_callback(void *_ctx, void(*cb)(void *), void *user_data)
{
    LinuxWsContext *ctx = (LinuxWsContext *)_ctx;

    ctx->pong_cb = cb;
    ctx->pong_cb_userdata = user_data;
}

void platform_ws_register_connection_error_callback(void *_ctx, void (*cb)(PlatformConnectionError, void *), void *user_data)
{
    LinuxWsContext *ctx = (LinuxWsContext *)_ctx;

    ctx->conn_error_cb = cb;
    ctx->conn_error_cb_userdata = user_data;
}

void ws_tick() {
    if (ctx_count > 0)
        mg_mgr_poll(&mgr, 1);
}

#endif
