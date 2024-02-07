#if defined(OCPP_PLATFORM_LINUX ) || defined(OCPP_PLATFORM_TEST)

#include "LinuxWS.h"

#include "ocpp/Platform.h"

#define URL_PARSER_IMPLEMENTATION
#include "lib/url.h"

#include "mongoose.h"

#include <memory>

struct mg_mgr mgr;        // Event manager
struct mg_connection *c;  // Client connection
void(*recv_cb)(char *, size_t, void *) = nullptr;
void *recv_cb_userdata = nullptr;

void(*pong_cb)(void *) = nullptr;
void *pong_cb_userdata = nullptr;

bool connected = false;
bool done = false;        // Event handler flips it to true

bool is_ssl = false;

// Print websocket response and signal that we're done
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_CONNECT) {
    // If this is a wss:// connection, tell client connection to use TLS
    if (is_ssl) {
      log_warn("Certificates are not checked yet!");
      struct mg_tls_opts opts = {};
      mg_tls_init(c, &opts);
    }
 }
  else if (ev == MG_EV_OPEN) {
    //c->is_hexdumping = 1;

  } else if (ev == MG_EV_ERROR) {
    // On error, log error message
    MG_ERROR(("%p %s", c->fd, (char *) ev_data));
  } else if (ev == MG_EV_WS_OPEN) {
    // When websocket handshake is successful, send message
    connected = true;
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;

    auto useless_copy = std::unique_ptr<char[]>(new char[wm->data.len]);
    memcpy(useless_copy.get(), wm->data.ptr, wm->data.len);

    if (recv_cb != nullptr) {
        recv_cb(useless_copy.get(), wm->data.len, recv_cb_userdata);
    }
  } else if (ev == MG_EV_WS_CTL) {
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    uint8_t op = wm->flags & 15;
    if (op == WEBSOCKET_OP_PONG)
        pong_cb(pong_cb_userdata);
  }

  if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE) {
    *(bool *) fn_data = true;  // Signal that we're done
    connected = false;
  }
}

std::unique_ptr<char[]> websocket_url_buf;
std::unique_ptr<char[]> auth_header_buf;

static void platform_connect() {
    if (auth_header_buf != nullptr)
        c = mg_ws_connect(&mgr, websocket_url_buf.get(), fn, &done, "Sec-WebSocket-Protocol: ocpp1.6\r\nAuthorization: Basic %s\r\n", auth_header_buf.get());     // Create client
    else
        c = mg_ws_connect(&mgr, websocket_url_buf.get(), fn, &done, "%s", "Sec-WebSocket-Protocol: ocpp1.6\r\n");     // Create client
}

void* platform_init(const char *websocket_url, const char *basic_auth_user, const uint8_t *basic_auth_pass, size_t basic_auth_pass_length)
{
    mg_mgr_init(&mgr);        // Initialise event manager
    //mg_log_set("4");
    is_ssl = mg_url_is_ssl(websocket_url);

    auto url_len = strlen(websocket_url);
    websocket_url_buf = heap_alloc_array<char>(url_len + 1);
    memcpy(websocket_url_buf.get(), websocket_url, url_len + 1); // copy with null-terminator

    if (basic_auth_user != nullptr && basic_auth_pass != nullptr) {
        auth_header_buf = heap_alloc_array<char>(2 * (strlen(basic_auth_user) + basic_auth_pass_length + 1) + 1);

        int offset = 0;
        for(int i = 0; i < strlen(basic_auth_user); ++i)
            offset = mg_base64_update(basic_auth_user[i], auth_header_buf.get(), offset);

        offset = mg_base64_update(':', auth_header_buf.get(), offset);

        for(int i = 0; i < basic_auth_pass_length; ++i)
            offset = mg_base64_update(basic_auth_pass[i], auth_header_buf.get(), offset);

        offset = mg_base64_final(auth_header_buf.get(), offset);
    }

    platform_connect();

    return &mgr;
}

void platform_disconnect(void *ctx) {
    if (!connected)
        return;

    mg_ws_send(c, "", 0, WEBSOCKET_OP_CLOSE);
    c->is_draining = 1;

    while(connected)
        mg_mgr_poll(&mgr, 1);
}

void platform_reconnect(void *ctx) {
    platform_disconnect(ctx);
    platform_connect();
}

void platform_destroy(void *ctx) {
    mg_mgr_free(&mgr);
}

bool platform_ws_connected(void *ctx)
{
    return connected;
}

bool platform_ws_send(void *ctx, const char *buf, size_t buf_len)
{
    mg_ws_send(c, buf, buf_len, WEBSOCKET_OP_TEXT) == buf_len;
}

bool platform_ws_send_ping(void *ctx) {
    mg_ws_send(c, "", 0, WEBSOCKET_OP_PING);
    return true;
}

void platform_ws_register_receive_callback(void *ctx, void(*cb)(char *, size_t, void *), void *user_data)
{
    recv_cb = cb;
    recv_cb_userdata = user_data;
}

void platform_ws_register_pong_callback(void *ctx, void(*cb)(void *), void *user_data)
{
    pong_cb = cb;
    pong_cb_userdata = user_data;
}

void ws_tick() {
    mg_mgr_poll(&mgr, 1);
}

#endif
