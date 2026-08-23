#if defined(OCPP_PLATFORM_LINUX) || defined(OCPP_PLATFORM_TEST) || defined(OCPP_PLATFORM_LINUX21)

#include "LinuxWS.h"

#include <common/Platform.h>
#include <common/Tools.h>

#define URL_PARSER_IMPLEMENTATION
#include "lib/url.h"

#include "mongoose.h"

#include <memory>
#include <string>
#include <vector>

// The mongoose event manager is shared by all websocket contexts.
static struct mg_mgr mgr;
static size_t ctx_count = 0;

struct LinuxWsContext {
    struct mg_connection *conn = nullptr;
    std::unique_ptr<char[]> url;
    std::string subprotocol;
    std::vector<std::unique_ptr<char[]>> auth_header_bufs;
    size_t next_auth_header = 0;
    bool connected = false;
    bool is_ssl = false;

    void (*recv_cb)(char *, size_t, void *) = nullptr;
    void *recv_cb_userdata = nullptr;

    void (*pong_cb)(void *) = nullptr;
    void *pong_cb_userdata = nullptr;
};

static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    LinuxWsContext *ctx = (LinuxWsContext *)fn_data;

    if (ev == MG_EV_CONNECT) {
        // If this is a wss:// connection, tell client connection to use TLS
        if (ctx->is_ssl) {
            log_warn("Certificates are not checked yet!");
            struct mg_tls_opts opts = {};
            mg_tls_init(c, &opts);
        }
    } else if (ev == MG_EV_ERROR) {
        MG_ERROR(("%p %s", c->fd, (char *) ev_data));
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

void *platform_init(const char *websocket_url, const char *subprotocol, BasicAuthCredentials *credentials, size_t credentials_length)
{
    if (ctx_count == 0)
        mg_mgr_init(&mgr);
    ++ctx_count;

    auto ctx = new LinuxWsContext();

    ctx->is_ssl = mg_url_is_ssl(websocket_url);
    ctx->subprotocol = subprotocol;

    auto url_len = strlen(websocket_url);
    ctx->url = heap_alloc_array<char>(url_len + 1);
    memcpy(ctx->url.get(), websocket_url, url_len + 1); // copy with null-terminator

    if (credentials != nullptr) {
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

    platform_connect(ctx);

    return ctx;
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

void ws_tick() {
    if (ctx_count > 0)
        mg_mgr_poll(&mgr, 1);
}

#endif
