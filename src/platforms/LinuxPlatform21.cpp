// Linux host platform for the OCPP 2.1 layer. Provides main() and the
// platform functions needed by src/ocpp21. The full 1.6 LinuxPlatform16.cpp
// is not used here because it implements the 1.6 EVSE/meter/persistency
// hooks that the 2.1 layer does not have yet.

#ifdef OCPP_PLATFORM_LINUX21

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ocpp21/ChargePoint21.h"
#include <common/Platform.h>

#define TFJSON_IMPLEMENTATION
#include "TFJson.h"

#include "LinuxWS.h"

uint32_t platform_now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000L);
}

static time_t last_system_time = 0;
static uint32_t last_system_time_set_at = 0;

void platform_set_system_time(void *ctx, time_t t)
{
    (void)ctx;
    last_system_time = t;
    last_system_time_set_at = platform_now_ms();
}

time_t platform_get_system_time(void *ctx) {
    (void)ctx;
    return last_system_time + (platform_now_ms() - last_system_time_set_at) / 1000;
}

void platform_printfln(int level, const char *fmt, ...)
{
    switch (level) {
        case OCPP_LOG_LEVEL_ERROR: printf("[ERROR] "); break;
        case OCPP_LOG_LEVEL_WARN:  printf("[WARN ] "); break;
        case OCPP_LOG_LEVEL_INFO:  printf("[INFO ] "); break;
        case OCPP_LOG_LEVEL_DEBUG: printf("[DEBUG] "); break;
        case OCPP_LOG_LEVEL_TRACE: printf("[TRACE] "); break;
        default: break;
    }
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    puts("");
}

void platform_reset(bool hard) {
    (void)hard;
    log_info("Reset requested. (ignored on the Linux 2.1 host platform)");
}

[[gnu::noreturn]] void system_abort(const char *message) {
    fprintf(stderr, "abort: %s\n", message);
    abort();
}

const char *platform_get_charge_point_vendor() {
    return "Tinkerforge GmbH";
}

const char *platform_get_charge_point_model() {
    return "WARP4 (ocpp21 host)";
}

const char *platform_get_charge_point_serial_number() {
    return "warp4-ocpp21-host";
}

const char *platform_get_firmware_version() {
    return "0.1.0";
}

static Ocpp21::ChargePoint21 cp;
static Ocpp21::ChargePoint21 cp2;

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <websocket-endpoint-url> <charge-point-name> [basic-auth-pass] [charge-point-name-2] [basic-auth-pass-2]\n", argv[0]);
        fprintf(stderr, "A second charge point name starts a second, parallel client on the same endpoint.\n");
        return 1;
    }

    const char *pass = argc >= 4 ? argv[3] : nullptr;
    const char *name2 = argc >= 5 ? argv[4] : nullptr;
    const char *pass2 = argc >= 6 ? argv[5] : nullptr;

    setvbuf(stdout, nullptr, _IOLBF, 0);

    platform_set_system_time(nullptr, time(nullptr));

    if (!cp.start(argv[1], argv[2], pass)) {
        fprintf(stderr, "Failed to start charge point\n");
        return 1;
    }

    if (name2 != nullptr && !cp2.start(argv[1], name2, pass2)) {
        fprintf(stderr, "Failed to start second charge point\n");
        return 1;
    }

    while (true) {
        cp.tick();
        if (name2 != nullptr)
            cp2.tick();
        ws_tick();
        usleep(1000);
    }

    return 0;
}

#endif
