#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <time.h>

#include <memory>

// Core platform API shared by the OCPP 1.6 and OCPP 2.x stacks.
// Version specific hooks live in ocpp16/Platform.h and ocpp21/Platform21.h.

struct BasicAuthCredentials {
    std::unique_ptr<char[]> user;
    std::unique_ptr<uint8_t[]> pass;
    size_t pass_length;
};

void *platform_init(const char *websocket_url, const char *subprotocol, BasicAuthCredentials *credentials = nullptr, size_t credentials_length = 0);
void platform_disconnect(void *ctx);
void platform_reconnect(void *ctx);
void platform_destroy(void *ctx);

bool platform_ws_connected(void *ctx);
bool platform_ws_send(void *ctx, const char *buf, size_t buf_len);
void platform_ws_register_receive_callback(void *ctx, void(*cb)(char *, size_t, void *), void *user_data);
bool platform_ws_send_ping(void *ctx);
void platform_ws_register_pong_callback(void *ctx, void (*cb)(void *), void *user_data);

uint32_t platform_now_ms();
void platform_set_system_time(void *ctx, time_t t);
time_t platform_get_system_time(void *ctx);

#define OCPP_LOG_LEVEL_NONE 0
#define OCPP_LOG_LEVEL_ERROR 1
#define OCPP_LOG_LEVEL_WARN 2
#define OCPP_LOG_LEVEL_INFO 3
#define OCPP_LOG_LEVEL_DEBUG 4
#define OCPP_LOG_LEVEL_TRACE 5

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

#if OCPP_LOG_LEVEL >= OCPP_LOG_LEVEL_TRACE
#define log_trace(...) platform_printfln(OCPP_LOG_LEVEL_TRACE, __VA_ARGS__)
#else
#define log_trace(...)
#endif

void platform_printfln(int level, const char *fmt, ...) __attribute__((__format__(__printf__, 2, 3)));

void platform_reset(bool hard);

size_t platform_read_file(const char *name, char *buf, size_t len);
bool platform_write_file(const char *name, char *buf, size_t len);

// return nullptr if name does not exist or is not a directory
void *platform_open_dir(const char *name);

struct OcppDirEnt {
    bool is_dir;
    char name[33] = "";
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
