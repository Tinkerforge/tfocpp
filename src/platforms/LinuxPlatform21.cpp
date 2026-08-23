// Linux host platform for the OCPP 2.1 layer. Provides main() and the
// platform functions needed by src/ocpp21. The full 1.6 LinuxPlatform16.cpp
// is not used here because it implements the 1.6 EVSE/meter/persistency
// hooks that the 2.1 layer does not have yet.

#ifdef OCPP_PLATFORM_LINUX21

#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ocpp21/ChargePoint21.h"
#include "ocpp21/Platform21.h"
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

// Simulated EVSE, driven by stdin commands. One global simulator shared by
// all instances, sufficient for the host tests which use one instance.
struct SimEvse {
    EvseState21 state = EvseState21::NotConnected;
    bool charging_allowed = false;
    double energy_wh = 0;
    uint32_t last_energy_update = 0;
};

#define SIM_CHARGE_POWER_W 11000.0
#define SIM_NUM_EVSES 1

static SimEvse sim_evses[SIM_NUM_EVSES];

struct TagSeenCallback {
    void *ctx = nullptr;
    void (*cb)(int32_t, const char *, void *) = nullptr;
    void *user_data = nullptr;
};

static TagSeenCallback tag_seen_cbs[4];

void platform_register_tag_seen_callback21(void *ctx, void (*cb)(int32_t evse_id, const char *tag_id, void *user_data), void *user_data)
{
    for (auto &entry : tag_seen_cbs) {
        if (entry.cb != nullptr && entry.ctx != ctx)
            continue;
        entry.ctx = ctx;
        entry.cb = cb;
        entry.user_data = user_data;
        return;
    }
}

static void sim_update(SimEvse &e)
{
    uint32_t now = platform_now_ms();
    if (e.state == EvseState21::Charging)
        e.energy_wh += SIM_CHARGE_POWER_W * (double)(now - e.last_energy_update) / 3600000.0;
    e.last_energy_update = now;

    if (e.state == EvseState21::Connected && e.charging_allowed)
        e.state = EvseState21::Charging;
    if (e.state == EvseState21::Charging && !e.charging_allowed)
        e.state = EvseState21::Connected;
}

EvseState21 platform_get_evse_state21(void *ctx, int32_t evse_id)
{
    (void)ctx;
    if (evse_id < 1 || evse_id > SIM_NUM_EVSES)
        return EvseState21::Faulted;
    auto &e = sim_evses[evse_id - 1];
    sim_update(e);
    return e.state;
}

void platform_set_charging_allowed21(void *ctx, int32_t evse_id, bool allowed)
{
    (void)ctx;
    if (evse_id < 1 || evse_id > SIM_NUM_EVSES)
        return;
    auto &e = sim_evses[evse_id - 1];
    sim_update(e);
    e.charging_allowed = allowed;
    sim_update(e);
    printf("[SIM  ] EVSE %d power path %s\n", evse_id, allowed ? "closed" : "open");
}

float platform_get_energy_wh21(void *ctx, int32_t evse_id)
{
    (void)ctx;
    if (evse_id < 1 || evse_id > SIM_NUM_EVSES)
        return 0;
    auto &e = sim_evses[evse_id - 1];
    sim_update(e);
    return (float)e.energy_wh;
}

// Commands: plug, unplug, tag <id>, fault, ok
static void sim_handle_command(char *line)
{
    auto &e = sim_evses[0];
    sim_update(e);

    if (strcmp(line, "plug") == 0) {
        if (e.state == EvseState21::NotConnected)
            e.state = e.charging_allowed ? EvseState21::Charging : EvseState21::Connected;
        printf("[SIM  ] cable plugged\n");
    } else if (strcmp(line, "unplug") == 0) {
        e.state = EvseState21::NotConnected;
        printf("[SIM  ] cable unplugged\n");
    } else if (strncmp(line, "tag ", 4) == 0) {
        printf("[SIM  ] tag %s seen\n", line + 4);
        for (auto &entry : tag_seen_cbs) {
            if (entry.cb != nullptr) {
                entry.cb(1, line + 4, entry.user_data);
                break;
            }
        }
    } else if (strcmp(line, "fault") == 0) {
        e.state = EvseState21::Faulted;
        printf("[SIM  ] EVSE faulted\n");
    } else if (strcmp(line, "ok") == 0) {
        e.state = EvseState21::NotConnected;
        printf("[SIM  ] EVSE fault cleared\n");
    } else if (line[0] != '\0') {
        printf("[SIM  ] unknown command %s (plug, unplug, tag <id>, fault, ok)\n", line);
    }
}

static void sim_poll_stdin()
{
    static char line[128];
    static size_t used = 0;

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '\n') {
            line[used] = '\0';
            used = 0;
            sim_handle_command(line);
            continue;
        }
        if (used < sizeof(line) - 1)
            line[used++] = c;
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <websocket-endpoint-url> <charge-point-name> [basic-auth-pass] [charge-point-name-2] [basic-auth-pass-2]\n", argv[0]);
        fprintf(stderr, "A second charge point name starts a second, parallel client on the same endpoint.\n");
        fprintf(stderr, "EVSE simulation via stdin: plug, unplug, tag <id>, fault, ok\n");
        return 1;
    }

    const char *pass = argc >= 4 ? argv[3] : nullptr;
    const char *name2 = argc >= 5 ? argv[4] : nullptr;
    const char *pass2 = argc >= 6 ? argv[5] : nullptr;

    setvbuf(stdout, nullptr, _IOLBF, 0);
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    srand((unsigned)time(nullptr) ^ (unsigned)getpid());

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
        sim_poll_stdin();
        cp.tick();
        if (name2 != nullptr)
            cp2.tick();
        ws_tick();
        usleep(1000);
    }

    return 0;
}

#endif
