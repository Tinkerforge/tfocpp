#include "OcppPlatform.h"

#include "OcppChargePoint.h"
#include "time.h"

#define URL_PARSER_IMPLEMENTATION
#include "lib/url_parse/url.h"

#include "lib/mongoose/mongoose.h"

#include <memory>

#include <dirent.h>

struct mg_mgr mgr;        // Event manager
struct mg_connection *c;  // Client connection
void(*recv_cb)(char *, size_t, void *) = nullptr;
void *recv_cb_userdata = nullptr;

void (*stop_cb)(int32_t, StopReason, void *) = nullptr;
void *stop_cb_userdata = nullptr;

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

    auto useless_copy = std::unique_ptr<char[]>(new char[wm->data.len]);
    memcpy(useless_copy.get(), wm->data.ptr, wm->data.len);

    if (recv_cb != nullptr) {
        recv_cb(useless_copy.get(), wm->data.len, recv_cb_userdata);
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
    if (!connected)
        return;

    mg_ws_send(c, "", 0, WEBSOCKET_OP_CLOSE);
    c->is_draining = 1;

    while(connected)
        mg_mgr_poll(&mgr, 1);
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

void platform_register_stop_callback(void *ctx, void(*cb)(int32_t, StopReason, void *), void *user_data)
{
    stop_cb = cb;
    stop_cb_userdata = user_data;
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

void platform_printfln(int level, const char *fmt, ...)
{
    switch (level) {
        case OCPP_LOG_LEVEL_ERROR:
            fputs("[ERROR]   ", stdout);
            break;
        case OCPP_LOG_LEVEL_WARN:
            fputs("[WARNING] ", stdout);
            break;
        case OCPP_LOG_LEVEL_INFO:
            fputs("[INFO]    ", stdout);
            break;
        case OCPP_LOG_LEVEL_DEBUG:
            fputs("[DEBUG]   ", stdout);
            break;
        default:
            break;
    }
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

// This is the Energy.Active.Import.Register measurant in Wh
int32_t platform_get_energy(int32_t connectorId) {
    return 1234;
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

int argc_;
char **argv_;

int main(int argc, char **argv) {
    argc_ = argc;
    argv_ = argv;

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

void platform_reset() {
    char *exec_argv[] = { argv_[0], 0 };

    execv("/proc/self/exe", exec_argv);
}

SupportedMeasurand supported_measurands[] = {
    //ENERGY_ACTIVE_EXPORT_REGISTER
    {SampledValuePhase::L1, SampledValueLocation::OUTLET, SampledValueUnit::K_WH, false},
    {SampledValuePhase::L2, SampledValueLocation::OUTLET, SampledValueUnit::K_WH, false},
    {SampledValuePhase::L3, SampledValueLocation::OUTLET, SampledValueUnit::K_WH, false},

    //ENERGY_ACTIVE_IMPORT_REGISTER
    {SampledValuePhase::L1, SampledValueLocation::OUTLET, SampledValueUnit::K_WH, false},
    {SampledValuePhase::L2, SampledValueLocation::OUTLET, SampledValueUnit::K_WH, false},
    {SampledValuePhase::L3, SampledValueLocation::OUTLET, SampledValueUnit::K_WH, false},

    //ENERGY_REACTIVE_EXPORT_REGISTER
    {SampledValuePhase::L1, SampledValueLocation::OUTLET, SampledValueUnit::KVARH, false},
    {SampledValuePhase::L2, SampledValueLocation::OUTLET, SampledValueUnit::KVARH, false},
    {SampledValuePhase::L3, SampledValueLocation::OUTLET, SampledValueUnit::KVARH, false},

    //ENERGY_REACTIVE_IMPORT_REGISTER
    {SampledValuePhase::L1, SampledValueLocation::OUTLET, SampledValueUnit::KVARH, false},
    {SampledValuePhase::L2, SampledValueLocation::OUTLET, SampledValueUnit::KVARH, false},
    {SampledValuePhase::L3, SampledValueLocation::OUTLET, SampledValueUnit::KVARH, false},

    //POWER_ACTIVE_EXPORT
    {SampledValuePhase::L1, SampledValueLocation::OUTLET, SampledValueUnit::W, false},
    {SampledValuePhase::L2, SampledValueLocation::OUTLET, SampledValueUnit::W, false},
    {SampledValuePhase::L3, SampledValueLocation::OUTLET, SampledValueUnit::W, false},

    //POWER_ACTIVE_IMPORT
    {SampledValuePhase::L1, SampledValueLocation::OUTLET, SampledValueUnit::W, false},
    {SampledValuePhase::L2, SampledValueLocation::OUTLET, SampledValueUnit::W, false},
    {SampledValuePhase::L3, SampledValueLocation::OUTLET, SampledValueUnit::W, false},

    //POWER_REACTIVE_EXPORT
    {SampledValuePhase::L1, SampledValueLocation::OUTLET, SampledValueUnit::W, false},
    {SampledValuePhase::L2, SampledValueLocation::OUTLET, SampledValueUnit::W, false},
    {SampledValuePhase::L3, SampledValueLocation::OUTLET, SampledValueUnit::W, false},

    //POWER_REACTIVE_IMPORT
    {SampledValuePhase::L1, SampledValueLocation::OUTLET, SampledValueUnit::W, false},
    {SampledValuePhase::L2, SampledValueLocation::OUTLET, SampledValueUnit::W, false},
    {SampledValuePhase::L3, SampledValueLocation::OUTLET, SampledValueUnit::W, false},

    //POWER_FACTOR
    {SampledValuePhase::L1, SampledValueLocation::OUTLET, SampledValueUnit::NONE, false},
    {SampledValuePhase::L2, SampledValueLocation::OUTLET, SampledValueUnit::NONE, false},
    {SampledValuePhase::L3, SampledValueLocation::OUTLET, SampledValueUnit::NONE, false},

    // We can measure this with 1. the offered current and 2. the connected phases

    //CURRENT_OFFERED
    {SampledValuePhase::L1, SampledValueLocation::OUTLET, SampledValueUnit::A, false},
    {SampledValuePhase::L2, SampledValueLocation::OUTLET, SampledValueUnit::A, false},
    {SampledValuePhase::L3, SampledValueLocation::OUTLET, SampledValueUnit::A, false},

    //VOLTAGE
    {SampledValuePhase::L1_N, SampledValueLocation::OUTLET, SampledValueUnit::V, false},
    {SampledValuePhase::L2_N, SampledValueLocation::OUTLET, SampledValueUnit::V, false},
    {SampledValuePhase::L3_N, SampledValueLocation::OUTLET, SampledValueUnit::V, false},
    {SampledValuePhase::L1_L2, SampledValueLocation::OUTLET, SampledValueUnit::V, false},
    {SampledValuePhase::L2_L3, SampledValueLocation::OUTLET, SampledValueUnit::V, false},
    {SampledValuePhase::L3_L1, SampledValueLocation::OUTLET, SampledValueUnit::V, false},

    //FREQUENCY
    /*
    NOTE: OCPP 1.6 does not have a UnitOfMeasure for
    frequency, the UnitOfMeasure for any SampledValue with measurand: Frequency is Hertz.
    */
    {SampledValuePhase::NONE, SampledValueLocation::OUTLET, SampledValueUnit::NONE, false},
};

size_t supported_measurand_offsets[] = {
    0,  /*ENERGY_ACTIVE_EXPORT_REGISTER*/
    3,  /*ENERGY_ACTIVE_IMPORT_REGISTER*/
    6,  /*ENERGY_REACTIVE_EXPORT_REGISTER*/
    9,  /*ENERGY_REACTIVE_IMPORT_REGISTER*/
    12, /*ENERGY_ACTIVE_EXPORT_INTERVAL*/
    12, /*ENERGY_ACTIVE_IMPORT_INTERVAL*/
    12, /*ENERGY_REACTIVE_EXPORT_INTERVAL*/
    12, /*ENERGY_REACTIVE_IMPORT_INTERVAL*/
    12, /*POWER_ACTIVE_EXPORT*/
    15, /*POWER_ACTIVE_IMPORT*/
    18, /*POWER_OFFERED*/
    18, /*POWER_REACTIVE_EXPORT*/
    21, /*POWER_REACTIVE_IMPORT*/
    24, /*POWER_FACTOR*/
    27, /*CURRENT_IMPORT*/
    27, /*CURRENT_EXPORT*/
    27, /*CURRENT_OFFERED*/
    30, /*VOLTAGE*/
    36, /*FREQUENCY*/
    37, /*TEMPERATURE*/
    37, /*SO_C*/
    37, /*RPM*/
    37  /*NONE*/
};

size_t platform_get_supported_measurand_count(int32_t connector_id, SampledValueMeasurand measurand) {
    if (connector_id == 0)
        return 0;
    if (measurand == SampledValueMeasurand::NONE)
        return ARRAY_SIZE(supported_measurands);
    return supported_measurand_offsets[(size_t)measurand + 1] - supported_measurand_offsets[(size_t)measurand];
}

const SupportedMeasurand *platform_get_supported_measurands(int32_t connector_id, SampledValueMeasurand measurand) {
    if (connector_id == 0)
        return nullptr;
    if (measurand == SampledValueMeasurand::NONE)
        return supported_measurands;
    return supported_measurands + supported_measurand_offsets[(size_t)measurand];
}

bool platform_get_signed_meter_value(int32_t connectorId, SampledValueMeasurand measurant, SampledValuePhase phase, SampledValueLocation location, char buf[PLATFORM_MEASURAND_MAX_DATA_LEN]) {
    log_warn("signed values not supported yet!");
}

float platform_get_raw_meter_value(int32_t connectorId, SampledValueMeasurand measurant, SampledValuePhase phase, SampledValueLocation location) {
    return 123.456f;
}

size_t platform_read_file(const char *name, char *buf, size_t len)
{
    auto fd = open(name, O_RDONLY | O_CREAT, 0644);
    auto result = read(fd, buf, len);
    close(fd);
    return result < 0 ? 0 : result;
}

bool platform_write_file(const char *name, char *buf, size_t len)
{
    auto fd = open(name, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    auto written = write(fd, buf, len);
    close(fd);
    return written == len;
}

void* platform_open_dir(const char *name)
{
    return opendir(name);
}

static OcppDirEnt dir_ent;

OcppDirEnt* platform_read_dir(void *dir_fd)
{
    auto *d = readdir((DIR *)dir_fd);
    if (d == nullptr)
        return nullptr;

    dir_ent.is_dir = d->d_type != DT_REG;
    memset(dir_ent.name, 0, sizeof(dir_ent.name));
    strncpy(dir_ent.name, d->d_name, sizeof(dir_ent.name));
    dir_ent.name[sizeof(dir_ent.name) - 1] = '\0';
    return &dir_ent;
}

void platform_close_dir(void *dir_fd)
{
    closedir((DIR *)dir_fd);
}

void platform_remove_file(const char *name)
{
    unlink(name);
}

const char *platform_get_charge_point_vendor() {
    return "Tinkerforge GmbH";
}
const char *platform_get_charge_point_model() {
    return "WARP2 Charger Pro";
}

const char *platform_get_charge_point_serial_number() {
    return "warp2-X8A";
}
const char *platform_get_firmware_version() {
    return nullptr;
}
const char *platform_get_iccid() {
    return nullptr;
}
const char *platform_get_imsi() {
    return nullptr;
}
const char *platform_get_meter_type() {
    return nullptr;
}
const char *platform_get_meter_serial_number() {
    return nullptr;
}
