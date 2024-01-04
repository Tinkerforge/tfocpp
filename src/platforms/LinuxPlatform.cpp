#ifdef OCPP_PLATFORM_LINUX

#include "LinuxWS.h"

#include "ocpp/Platform.h"

#include "ocpp/ChargePoint.h"
#include "time.h"

#include <dirent.h>

#define TFJSON_IMPLEMENTATION
#include "TFJson.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

struct PlatformResponse {
    uint8_t seq_num;
    char tag_id_seen[22];
    uint8_t fixed_cable;
    static_assert(OCPP_NUM_CONNECTORS <= 8, "Linux platform only supports up to 8 connectors for now. Change fixed cable in PlatformResponse to increase this.");
    uint8_t evse_state[OCPP_NUM_CONNECTORS];
    uint32_t energy[OCPP_NUM_CONNECTORS];
}  __attribute__((__packed__));

struct ConnectorMessage {
    uint8_t state;
    uint8_t last_sent_status;
    char tag_id[21];
    char parent_tag_id[21];
    uint8_t tag_status;
    time_t tag_expiry_date;
    uint32_t tag_deadline;
    uint32_t cable_deadline;
    int32_t txn_id;
    time_t transaction_confirmed_timestamp;
    time_t transaction_start_time;
    uint32_t current_allowed;
    bool txn_with_invalid_id;
    bool unavailable_requested;
}  __attribute__((__packed__));

struct PlatformMessage {
    uint8_t seq_num = 0;
    char message[63] = "";
    uint32_t charge_current[OCPP_NUM_CONNECTORS] = {0};
    uint8_t connector_locked = 0;

    uint8_t charge_point_state;
    uint8_t charge_point_last_sent_status;
    time_t next_profile_eval;

    uint8_t message_in_flight_type;
    uint64_t message_in_flight_id;
    size_t message_in_flight_len;
    uint32_t message_timeout_deadline;
    uint32_t txn_msg_retry_deadline;
    uint8_t message_queue_depth;
    uint8_t status_notification_queue_depth;
    uint8_t transaction_message_queue_depth;

    uint8_t config_key;
    char config_value[500];

    ConnectorMessage connector_messages[OCPP_NUM_CONNECTORS];
}  __attribute__((__packed__));

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

void (*stop_cb)(int32_t, StopReason, void *) = nullptr;
void *stop_cb_userdata = nullptr;

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
    auto timestamp = platform_get_system_time(nullptr);
    char buf[OCPP_ISO_8601_MAX_LEN] = {0};
    const tm *t = localtime(&timestamp);

    strftime(buf, ARRAY_SIZE(buf), "%F %T ", t);

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
        case OCPP_LOG_LEVEL_TRACE:
            fputs("[TRACE]   ", stdout);
            break;
        default:
            break;
    }

    fputs(buf, stdout);

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

const char * const trt_string[] = {
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

#ifdef OCPP_STATE_CALLBACKS
void platform_update_chargepoint_state(OcppState state,
                                       StatusNotificationStatus last_sent_status,
                                       time_t next_profile_eval) {
    pm.charge_point_state = (uint8_t) state;
    pm.charge_point_last_sent_status = (uint8_t) last_sent_status;
    pm.next_profile_eval = next_profile_eval;
    send_message("debug chargepoint");
}

void platform_update_connector_state(int32_t connector_id,
                                     ConnectorState state,
                                     StatusNotificationStatus last_sent_status,
                                     IdTagInfo auth_for,
                                     uint32_t tag_deadline,
                                     uint32_t cable_deadline,
                                     int32_t txn_id,
                                     time_t transaction_confirmed_timestamp,
                                     time_t transaction_start_time,
                                     uint32_t current_allowed,
                                     bool txn_with_invalid_id,
                                     bool unavailable_requested) {
    auto &cm = pm.connector_messages[connector_id - 1];
    cm.state = (uint8_t) state;
    cm.last_sent_status = (uint8_t) last_sent_status;
    memcpy(cm.tag_id, auth_for.tagId, sizeof(cm.tag_id));
    memcpy(cm.parent_tag_id, auth_for.parentTagId, sizeof(cm.parent_tag_id));
    cm.tag_status = (uint8_t) auth_for.status;
    cm.tag_expiry_date = auth_for.expiryDate;
    cm.tag_deadline = tag_deadline;
    cm.cable_deadline = cable_deadline;
    cm.txn_id = txn_id;
    cm.transaction_confirmed_timestamp = transaction_confirmed_timestamp;
    cm.transaction_start_time = transaction_start_time;
    cm.current_allowed = current_allowed;
    cm.txn_with_invalid_id = txn_with_invalid_id;
    cm.unavailable_requested = unavailable_requested;
}

void platform_update_connection_state(CallAction message_in_flight_type,
                                      uint64_t message_in_flight_id,
                                      size_t message_in_flight_len,
                                      uint32_t message_timeout_deadline,
                                      uint32_t txn_msg_retry_deadline,
                                      uint8_t message_queue_depth,
                                      uint8_t status_notification_queue_depth,
                                      uint8_t transaction_message_queue_depth,
                                      bool connected,
                                      time_t connected_change_time,
                                      uint32_t last_ping_sent,
                                      uint32_t pong_deadline) {
    pm.message_in_flight_type = (uint8_t) message_in_flight_type;
    pm.message_in_flight_id = message_in_flight_id;
    pm.message_in_flight_len = message_in_flight_len;
    pm.message_timeout_deadline = message_timeout_deadline;
    pm.txn_msg_retry_deadline = txn_msg_retry_deadline;
    pm.message_queue_depth = message_queue_depth;
    pm.status_notification_queue_depth = status_notification_queue_depth;
    pm.transaction_message_queue_depth = transaction_message_queue_depth;
    send_message("debug connection");
}

void platform_update_config_state(ConfigKey key,
                                  const char *value) {
    pm.config_key = (uint8_t) key;
    strncpy(pm.config_value, value, sizeof(pm.config_value));
    send_message("debug config");
}
#endif

int argc_;
char **argv_;

int main(int argc, char **argv) {
    argc_ = argc;
    argv_ = argv;

    if (argc < 2 || argc > 3) {
        printf("Usage %s ws[s]://central-host-or-ip:port/central/path [basic_auth_pass]\n", argv[0]);
        return -1;
    }


    OcppChargePoint cp;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(34128);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(sock, (const sockaddr *) &addr, sizeof(addr));

    addr.sin_addr.s_addr = inet_addr("127.0.0.2");

    char buf[sizeof(PlatformResponse)] = {0};

    cp.start(argv[1], "CP_4", argc == 3 ? (const uint8_t *)argv[2] : nullptr, argc == 3 ? strlen(argv[2]) : 0);

    while(true) {
        cp.tick();
        ws_tick();
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

void platform_reset(bool hard) {
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

bool platform_get_signed_meter_value(int32_t connectorId, SampledValueMeasurand measurant, SampledValuePhase phase, SampledValueLocation location, char buf[OCPP_PLATFORM_MEASURAND_MAX_DATA_LEN]) {
    log_warn("signed values not supported yet!");
    return false;
}

float platform_get_raw_meter_value(int32_t connectorId, SampledValueMeasurand measurant, SampledValuePhase phase, SampledValueLocation location) {
    return 123.456f;
}

size_t platform_read_file(const char *name, char *buf, size_t len)
{
    auto fd = open(name, O_RDONLY, 0644);
    if (fd < 0)
        return 0;
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
    strncpy(dir_ent.name, d->d_name, sizeof(dir_ent.name) - 1);
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

uint32_t platform_get_maximum_charging_current(int32_t connectorId) {
    return 32000;
}

bool platform_has_fixed_cable(int32_t connectorId) {
    return pr.fixed_cable & (1 << connectorId);
}
#endif
