#if defined(OCPP_PLATFORM_LINUX) || defined(OCPP_PLATFORM_TEST)

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
size_t next_auth_header = 0;
std::vector<std::unique_ptr<char[]>> auth_header_bufs;

static void platform_connect() {
    if (auth_header_bufs.size() > 0) {
        c = mg_ws_connect(&mgr, websocket_url_buf.get(), fn, &done, "Sec-WebSocket-Protocol: ocpp1.6\r\nAuthorization: Basic %s\r\n", auth_header_bufs[next_auth_header].get());     // Create client
        next_auth_header = (next_auth_header + 1) % auth_header_bufs.size();
    } else
        c = mg_ws_connect(&mgr, websocket_url_buf.get(), fn, &done, "%s", "Sec-WebSocket-Protocol: ocpp1.6\r\n");     // Create client
}

void* platform_init(const char *websocket_url, BasicAuthCredentials *credentials, size_t credentials_length)
{
    mg_mgr_init(&mgr);        // Initialise event manager
    //mg_log_set("4");
    is_ssl = mg_url_is_ssl(websocket_url);

    auto url_len = strlen(websocket_url);
    websocket_url_buf = heap_alloc_array<char>(url_len + 1);
    memcpy(websocket_url_buf.get(), websocket_url, url_len + 1); // copy with null-terminator

    if (credentials != nullptr) {
        for (size_t cred_idx = 0; cred_idx < credentials_length; ++cred_idx) {
            auto auth_header_buf = heap_alloc_array<char>(2 * (strlen(credentials[cred_idx].user) + credentials[cred_idx].pass_length + 1) + 1);

            int offset = 0;
            for(int i = 0; i < strlen(credentials[cred_idx].user); ++i)
                offset = mg_base64_update(credentials[cred_idx].user[i], auth_header_buf.get(), offset);

            offset = mg_base64_update(':', auth_header_buf.get(), offset);

            for(int i = 0; i < credentials[cred_idx].pass_length; ++i)
                offset = mg_base64_update(credentials[cred_idx].pass[i], auth_header_buf.get(), offset);

            offset = mg_base64_final(auth_header_buf.get(), offset);
            auth_header_bufs.push_back(std::move(auth_header_buf));
        }
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
    // mg_ws_send returns the sent bytes including the (variable length) web socket frame header.
    // Checking for success with mg_ws_send() == buf_len is thus not possible.
    // As the ws header length is unknown, we can't check for == buf_len + [hardcoded] header len
    // -> Detecting a short write is not possible.
    // However ms_ws_send internally does not check for short writes and just reurns buf_len + header_len.
    // Assume that sending data always succeeds.
    mg_ws_send(c, buf, buf_len, WEBSOCKET_OP_TEXT);
    return true;
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
