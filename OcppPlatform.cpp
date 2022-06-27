#include "OcppPlatform.h"

#include "OcppChargePoint.h"
#include "time.h"

#define URL_PARSER_IMPLEMENTATION
#include "url_parse/url.h"

#include "mongoose/mongoose.h"

struct mg_mgr mgr;        // Event manager
struct mg_connection *c;  // Client connection
void(*recv_cb)(char *, size_t, void *) = nullptr;
void *recv_cb_userdata = nullptr;
bool connected = false;
bool done = false;        // Event handler flips it to true

// Print websocket response and signal that we're done
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_OPEN) {
    //c->is_hexdumping = 1;

  } else if (ev == MG_EV_ERROR) {
    // On error, log error message
    MG_ERROR(("%p %s", c->fd, (char *) ev_data));
  } else if (ev == MG_EV_WS_OPEN) {
    // When websocket handshake is successful, send message
    connected = true;
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;

    char *useless_copy = (char *)malloc(wm->data.len);
    memcpy(useless_copy, wm->data.ptr, wm->data.len);

    if (recv_cb != nullptr) {
        recv_cb(useless_copy, wm->data.len, recv_cb_userdata);
    }
  }

  if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE) {
    *(bool *) fn_data = true;  // Signal that we're done
    connected = false;
  }
}

PlatformMessage pm;
int sock;
struct sockaddr_in addr;

void send_message(const char *message) {
    pm.seq_num = (pm.seq_num + 1) % 256;
    memset(pm.message, 0, ARRAY_SIZE(pm.message));
    strncpy(pm.message, message, ARRAY_SIZE(pm.message));

    sendto(sock, &pm, sizeof(pm), 0, (const sockaddr *) &addr, sizeof(addr));
}

PlatformResponse pr;

void* platform_init(const char *websocket_url)
{
    mg_mgr_init(&mgr);        // Initialise event manager
    //mg_log_set("4");
    c = mg_ws_connect(&mgr, websocket_url, fn, &done, "%s", "Sec-WebSocket-Protocol: ocpp1.6\r\n");     // Create client
    return &mgr;
}

void platform_disconnect(void *ctx) {
    mg_ws_send(c, "", 0, WEBSOCKET_OP_CLOSE);
}

void platform_destroy(void *ctx) {
    mg_mgr_free(&mgr);
}

bool platform_ws_connected(void *ctx)
{
    return connected;
}

void platform_ws_send(void *ctx, const char *buf, size_t buf_len)
{
    mg_ws_send(c, buf, buf_len, WEBSOCKET_OP_TEXT);
}

void platform_ws_register_receive_callback(void *ctx, void(*cb)(char *, size_t, void *), void *user_data)
{
    recv_cb = cb;
    recv_cb_userdata = user_data;
}

uint32_t platform_now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts );
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000L);
}

time_t last_system_time = 0;
uint32_t last_system_time_set_at = 0;

void platform_set_system_time(void *ctx, time_t t)
{
    last_system_time = t;
    last_system_time_set_at = platform_now_ms();
}

time_t platform_get_system_time(void *ctx) {
    return last_system_time + (platform_now_ms() - last_system_time_set_at) / 1000;
}

void platform_printfln(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    puts("");
}

void(*tag_seen_cb)(int32_t, const char *, void *) = nullptr;
void *tag_seen_cb_user_data = nullptr;

void platform_register_tag_seen_callback(void *ctx, void(*cb)(int32_t, const char *, void *), void *user_data) {
    tag_seen_cb = cb;
    tag_seen_cb_user_data = user_data;
}

const char *trt_string[] = {
"Blocked",
"Expired",
"Invalid",
"ConcurrentTx",
};

void platform_tag_rejected(const char *tag, TagRejectionType trt) {
    char buf[61] = {0};

    snprintf(buf, ARRAY_SIZE(buf), "Tag %s rejected: %s", tag, trt_string[(size_t)trt]);
    send_message(buf);
}

void platform_tag_timed_out(int32_t connectorId)
{
    send_message("Tag timeout!");
}

void platform_cable_timed_out(int32_t connectorId)
{
    send_message("Cable timeout!");
}

EVSEState platform_get_evse_state(int32_t connectorId) {
    return (EVSEState)pr.evse_state[connectorId - 1];
}

/*
Value as a “Raw” (decimal) number or “SignedData”. Field Type is
“string” to allow for digitally signed data readings. Decimal numeric values are
also acceptable to allow fractional values for measurands such as Temperature
and Current.
*/
const char * platform_get_meter_value(int32_t connectorId, SampledValueMeasurand measurant) {
    printf("platform_get_meter_value not implemented yet!\n");
    return "";
}

// This is the Energy.Active.Import.Register measurant in Wh
int32_t platform_get_energy(int32_t connectorId) {
    printf("platform_get_energy not implemented yet!\n");
    return 0;
}

void platform_lock_cable(int32_t connectorId)
{
    pm.connector_locked |= (1 << (connectorId - 1));
    send_message("Locked connector");
}

void platform_unlock_cable(int32_t connectorId)
{
    pm.connector_locked &= ~(1 << (connectorId - 1));
    send_message("Unlocked connector");
}

void platform_set_charging_current(int32_t connectorId, uint32_t milliAmps)
{
    pm.charge_current[connectorId - 1] = milliAmps;
    send_message("Set charge current");
}


int main(int argc, char **argv) {
    OcppChargePoint cp;

    cp.start("ws://localhost:8180/steve/websocket/CentralSystemService", "CP_1");

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(34128);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(sock, (const sockaddr *) &addr, sizeof(addr));

    addr.sin_addr.s_addr = inet_addr("127.0.0.2");

    char buf[sizeof(PlatformResponse)] = {0};

    while(true) {
        cp.tick();
        mg_mgr_poll(&mgr, 1);
        for(int i = 0; i < 10; ++i)
            usleep(100);

        send_message("POLL");
        recv(sock, buf, ARRAY_SIZE(buf), 0);
        memcpy(&pr, buf, sizeof(pr));

        if (pr.tag_id_seen[0] != '\0')
            tag_seen_cb(pr.tag_id_seen[0], pr.tag_id_seen + 1, tag_seen_cb_user_data);
    }

    return 0;
}
