#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <time.h>

#include <functional>

#include "OcppMessages.h"

struct PlatformResponse {
    uint8_t seq_num;
    char tag_id_seen[22];
    uint8_t evse_state[8];
    uint32_t energy[8];
}  __attribute__((__packed__));

struct PlatformMessage {
    uint8_t seq_num = 0;
    char message[63] = "";
    uint32_t charge_current[8] = {0};
    uint8_t connector_locked = 0;
}  __attribute__((__packed__));

void *platform_init(const char *websocket_url);
void platform_disconnect(void *ctx);
void platform_destroy(void *ctx);

bool platform_ws_connected(void *ctx);
void platform_ws_send(void *ctx, const char *buf, size_t buf_len);
void platform_ws_register_receive_callback(void *ctx, void(*cb)(char *, size_t, void *), void *user_data);

uint32_t platform_now_ms();
void platform_set_system_time(void *ctx, time_t t);
time_t platform_get_system_time(void *ctx);

#define OCPP_LOG_LEVEL_NONE 0
#define OCPP_LOG_LEVEL_ERROR 1
#define OCPP_LOG_LEVEL_WARN 2
#define OCPP_LOG_LEVEL_INFO 3
#define OCPP_LOG_LEVEL_DEBUG 4

#if OCPP_LOG_LEVEL >= OCPP_LOG_LEVEL_ERROR
#define log_error(...) platform_printfln(OCPP_LOG_LEVEL_ERROR, __VA_ARGS__)
#else
#define log_error(...)
#endif

#if OCPP_LOG_LEVEL >= OCPP_LOG_LEVEL_WARN
#define log_warn(...) platform_printfln(OCPP_LOG_LEVEL_WARN, __VA_ARGS__)
#else
#define log_warn(...)
#endif

#if OCPP_LOG_LEVEL >= OCPP_LOG_LEVEL_INFO
#define log_info(...) platform_printfln(OCPP_LOG_LEVEL_INFO, __VA_ARGS__)
#else
#define log_info(...)
#endif

#if OCPP_LOG_LEVEL >= OCPP_LOG_LEVEL_DEBUG
#define log_debug(...) platform_printfln(OCPP_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define log_debug(...)
#endif

void platform_printfln(int level, const char *fmt, ...) __attribute__((__format__(__printf__, 2, 3)));

void platform_register_tag_seen_callback(void *ctx, void(*cb)(int32_t, const char *, void *), void *user_data);

enum TagRejectionType {
    Blocked,
    Expired,
    Invalid,
    ConcurrentTx
};

void platform_tag_rejected(const char *tag, TagRejectionType trt);
void platform_tag_timed_out(int32_t connectorId);
void platform_cable_timed_out(int32_t connectorId);

void platform_lock_cable(int32_t connectorId);
void platform_unlock_cable(int32_t connectorId);

enum EVSEState {
    NotConnected,
    PlugDetected,
    Connected,
    ReadyToCharge,
    Charging,
    Faulted
};

EVSEState platform_get_evse_state(int32_t connectorId);

#define OCPP_PLATFORM_MAX_CHARGING_CURRENT 0xFFFFFFFF
void platform_set_charging_current(int32_t connectorId, uint32_t milliAmps);

enum StopReason {
    //DeAuthorized, // handled by OCPP
    EmergencyStop, // if EVSE emergency stop button is pressed
    //EVDisconnected, detected via platform_get_evse_state
    //HardReset, // handled by OCPP
    Local, // "normal" EVSE stop button
    Other,
    PowerLoss, // "Complete loss of power." maybe use this if contactor check fails (before contactor)
    Reboot, // "A locally initiated reset/reboot occurred. (for instance watchdog kicked in)"
    Remote, // "Stopped remotely on request of the user." maybe use this when stopping over the web interface?
    // SoftReset, // handled by OCPP
    // UnlockCommand, // handled by OCPP
};
// Calling the stop callback will _never_ unlock the cable if it is currently locked. The same tag (or one in the same group) is required to unlock the cable.
// For example to implement a remote stop that unlocks immediately, use platform_unlock_cable.
void platform_register_stop_callback(void *ctx, void (*cb)(int32_t, StopReason, void *), void *user_data);

#define PLATFORM_MEASURAND_MAX_DATA_LEN 16 // 11 digits, point, 3 digits, \0
#define PLATFORM_MEASURAND_ACQUISITION_INTERVAL_MS 1000 //Tune this depending on the meter/communication speed
/*
Value as a “Raw” (decimal) number or “SignedData”. Field Type is
“string” to allow for digitally signed data readings. Decimal numeric values are
also acceptable to allow fractional values for measurands such as Temperature
and Current.
*/
/*
If the connectorId is 0, it is associated with the
entire Charge Point. If the connectorId is 0 and the Measurand is energy related, the sample SHOULD be
taken from the main energy meter.
*/
// Only REGISTER values are allowed to be signed?
// We must calculate the average over all non-energy values
// this cannot be done if they are signed (intransparent) binary blobs.
bool platform_get_signed_meter_value(int32_t connectorId, SampledValueMeasurand measurand, SampledValuePhase phase, SampledValueLocation location, char buf[PLATFORM_MEASURAND_MAX_DATA_LEN]);
float platform_get_raw_meter_value(int32_t connectorId, SampledValueMeasurand measurand, SampledValuePhase phase, SampledValueLocation location);

// This is the Energy.Active.Import.Register measurand in Wh
int32_t platform_get_energy(int32_t connectorId);

void platform_reset();

struct SupportedMeasurand {
    const SampledValuePhase phase;
    const SampledValueLocation location;
    const SampledValueUnit unit;
    const bool is_signed;
};

// Return number of ALL supported measurands if measurand == NONE
size_t platform_get_supported_measurand_count(int32_t connector_id, SampledValueMeasurand measurand);
// Return ALL supported measurands if measurand == NONE
const SupportedMeasurand *platform_get_supported_measurands(int32_t connector_id, SampledValueMeasurand measurand = SampledValueMeasurand::NONE);

size_t platform_read_file(const char *name, char *buf, size_t len);
bool platform_write_file(const char *name, char *buf, size_t len);

// return nullptr if name does not exist or is not a directory
void *platform_open_dir(const char *name);

struct OcppDirEnt {
    bool is_dir;
    char name[32] = "";
};

// return nullptr if no more files
OcppDirEnt *platform_read_dir(void *dir_fd);
void platform_close_dir(void *dir_fd);

void platform_remove_file(const char *name);

// Required
const char *platform_get_charge_point_vendor();
const char *platform_get_charge_point_model();

// Optional - Return nullptr if not to be sent.
const char *platform_get_charge_point_serial_number();
const char *platform_get_firmware_version();
const char *platform_get_iccid();
const char *platform_get_imsi();
const char *platform_get_meter_type();
const char *platform_get_meter_serial_number();
