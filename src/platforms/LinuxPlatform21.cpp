// Linux host platform for the OCPP 2.1 layer. Provides main() and the
// platform functions needed by src/ocpp21. The full 1.6 LinuxPlatform16.cpp
// is not used here because it implements the 1.6 EVSE/meter/persistency
// hooks that the 2.1 layer does not have yet.

#ifdef OCPP_PLATFORM_LINUX21

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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
    if (fd < 0 && errno == ENOENT) {
        const char *slash = strrchr(name, '/');
        if (slash != nullptr) {
            char dir[256];
            size_t dir_len = (size_t)(slash - name);
            if (dir_len < sizeof(dir)) {
                memcpy(dir, name, dir_len);
                dir[dir_len] = '\0';
                mkdir(dir, 0755);
                fd = open(name, O_WRONLY | O_TRUNC | O_CREAT, 0644);
            }
        }
    }
    if (fd < 0)
        return false;
    auto written = write(fd, buf, len);
    close(fd);
    return written >= 0 && (size_t)written == len;
}

void *platform_open_dir(const char *name)
{
    return opendir(name);
}

OcppDirEnt *platform_read_dir(void *dir_fd)
{
    static OcppDirEnt entry;
    struct dirent *d;
    while ((d = readdir((DIR *)dir_fd)) != nullptr) {
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0) {
            continue;
        }
        entry.is_dir = d->d_type == DT_DIR;
        strncpy(entry.name, d->d_name, sizeof(entry.name) - 1);
        entry.name[sizeof(entry.name) - 1] = '\0';
        return &entry;
    }
    return nullptr;
}

void platform_close_dir(void *dir_fd)
{
    closedir((DIR *)dir_fd);
}

void platform_remove_file(const char *name)
{
    unlink(name);
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

static Ocpp21::ChargePoint cp;
static Ocpp21::ChargePoint cp2;

void platform_cert_store_changed21(void *ctx)
{
    (void)ctx;
    size_t v2g_roots = 0, oem_roots = 0, mo_roots = 0;
    int v2g_chain = 0, v2g20_chain = 0;
    for (const auto &e : cp.cert_store.all()) {
        switch (e.group) {
            case Ocpp21::CertGroup::V2GRoot: ++v2g_roots; break;
            case Ocpp21::CertGroup::OEMRoot: ++oem_roots; break;
            case Ocpp21::CertGroup::MORoot: ++mo_roots; break;
            case Ocpp21::CertGroup::V2GChain: v2g_chain = 1; break;
            case Ocpp21::CertGroup::V2G20Chain: v2g20_chain = 1; break;
            default: break;
        }
    }
    printf("[SIM  ] Cert store changed: v2g chain %d, v2g20 chain %d, v2g roots %zu, oem roots %zu, mo roots %zu\n",
           v2g_chain, v2g20_chain, v2g_roots, oem_roots, mo_roots);
}

// Simulated EVSE, driven by stdin commands. One global simulator shared by
// all instances, sufficient for the host tests which use one instance.
struct SimEvse {
    EvseState21 state = EvseState21::NotConnected;
    bool charging_allowed = false;
    bool ev_suspended = false;
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

struct StopCallback {
    void *ctx = nullptr;
    void (*cb)(int32_t, StopReason21, void *) = nullptr;
    void *user_data = nullptr;
};

static StopCallback stop_cbs[4];

void platform_register_stop_callback21(void *ctx, void (*cb)(int32_t evse_id, StopReason21 reason, void *user_data), void *user_data)
{
    for (auto &entry : stop_cbs) {
        if (entry.cb != nullptr && entry.ctx != ctx)
            continue;
        entry.ctx = ctx;
        entry.cb = cb;
        entry.user_data = user_data;
        return;
    }
}

void platform_lock_cable21(void *ctx, int32_t evse_id)
{
    (void)ctx;
    printf("[SIM  ] EVSE %d cable locked\n", evse_id);
}

void platform_unlock_cable21(void *ctx, int32_t evse_id)
{
    (void)ctx;
    printf("[SIM  ] EVSE %d cable unlocked\n", evse_id);
}

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

    bool ev_connected = e.state == EvseState21::Connected
                     || e.state == EvseState21::ReadyToCharge
                     || e.state == EvseState21::Charging;
    if (!ev_connected)
        return;

    if (!e.charging_allowed)
        e.state = EvseState21::Connected;
    else if (e.ev_suspended)
        e.state = EvseState21::ReadyToCharge;
    else
        e.state = EvseState21::Charging;
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

static const char * const tag_rejection_strings[] = {"Blocked", "Expired", "Invalid", "ConcurrentTx"};

void platform_tag_expected21(void *ctx, int32_t evse_id)
{
    (void)ctx;
    printf("[SIM  ] EVSE %d expects a tag\n", evse_id);
}

void platform_clear_tag_expected21(void *ctx, int32_t evse_id)
{
    (void)ctx;
    printf("[SIM  ] EVSE %d no longer expects a tag\n", evse_id);
}

void platform_tag_accepted21(void *ctx, int32_t evse_id, const char *tag)
{
    (void)ctx;
    printf("[SIM  ] EVSE %d tag %s accepted\n", evse_id, tag);
}

void platform_tag_rejected21(void *ctx, int32_t evse_id, const char *tag, TagRejectionType21 trt)
{
    (void)ctx;
    printf("[SIM  ] EVSE %d tag %s rejected (%s)\n", evse_id, tag, tag_rejection_strings[(size_t)trt]);
}

void platform_tag_timed_out21(void *ctx, int32_t evse_id)
{
    (void)ctx;
    printf("[SIM  ] EVSE %d tag timed out\n", evse_id);
}

// Commands: plug, unplug, detect, suspend, resume, tag <id>, stop <reason>, fault, ok, secevent <type>
static void sim_handle_command(char *line)
{
    auto &e = sim_evses[0];
    sim_update(e);

    if (strcmp(line, "plug") == 0) {
        if (e.state == EvseState21::NotConnected || e.state == EvseState21::PlugDetected) {
            e.state = EvseState21::Connected;
            e.ev_suspended = false;
            sim_update(e);
        }
        printf("[SIM  ] EV connected\n");
    } else if (strcmp(line, "detect") == 0) {
        // Cable plugged into the EVSE without an EV, or the EV side was
        // pulled while the plug stays in the socket.
        e.state = EvseState21::PlugDetected;
        printf("[SIM  ] plug detected, no EV\n");
    } else if (strcmp(line, "unplug") == 0) {
        e.state = EvseState21::NotConnected;
        printf("[SIM  ] cable unplugged\n");
    } else if (strcmp(line, "suspend") == 0) {
        e.ev_suspended = true;
        sim_update(e);
        printf("[SIM  ] EV suspended charging\n");
    } else if (strcmp(line, "resume") == 0) {
        e.ev_suspended = false;
        sim_update(e);
        printf("[SIM  ] EV resumed charging\n");
    } else if (strncmp(line, "tag ", 4) == 0) {
        printf("[SIM  ] tag %s seen\n", line + 4);
        for (auto &entry : tag_seen_cbs) {
            if (entry.cb != nullptr) {
                entry.cb(1, line + 4, entry.user_data);
                break;
            }
        }
    } else if (strncmp(line, "stop ", 5) == 0) {
        StopReason21 reason;
        if (strcmp(line + 5, "emergency") == 0)
            reason = StopReason21::EmergencyStop;
        else if (strcmp(line + 5, "local") == 0)
            reason = StopReason21::Local;
        else if (strcmp(line + 5, "powerloss") == 0)
            reason = StopReason21::PowerLoss;
        else if (strcmp(line + 5, "reboot") == 0)
            reason = StopReason21::Reboot;
        else if (strcmp(line + 5, "remote") == 0)
            reason = StopReason21::Remote;
        else
            reason = StopReason21::Other;
        printf("[SIM  ] local stop (%s)\n", line + 5);
        for (auto &entry : stop_cbs) {
            if (entry.cb != nullptr) {
                entry.cb(1, reason, entry.user_data);
                break;
            }
        }
    } else if (strncmp(line, "secevent ", 9) == 0) {
        printf("[SIM  ] queueing security event %s\n", line + 9);
        cp.sendSecurityEventNotification(line + 9, "triggered via host simulator");
    } else if (strcmp(line, "m07") == 0 || strncmp(line, "m07 ", 4) == 0) {
        // Drives the vehicle chain OCSP plumbing until the ISO 15118 stack exists.
        // m07 [count]: chain of count certificates, leaf first (HUB20-432-006).
        size_t count = 1;
        if (line[3] == ' ') {
            long parsed = strtol(line + 4, nullptr, 10);
            if (parsed >= 1 && parsed <= 4)
                count = (size_t)parsed;
        }
        printf("[SIM  ] requesting vehicle chain OCSP status for %zu certificates\n", count);
        static const char * const serials[] = {"1234", "5678", "9abc", "def0"};
        OcppCertHashData21 hashes[4];
        const char *urls[4];
        memset(hashes, 0, sizeof(hashes));
        for (size_t c = 0; c < count; ++c) {
            for (size_t i = 0; i < 64; ++i) {
                hashes[c].issuer_name_hash[i] = 'a';
                hashes[c].issuer_key_hash[i] = 'b';
            }
            strcpy(hashes[c].serial_number, serials[c]);
            urls[c] = "http://127.0.0.1:1/ocsp";
        }
        cp.requestVehicleChainStatus(hashes, urls, count);
    } else if (strcmp(line, "evcert") == 0 || strcmp(line, "evcert update") == 0) {
        // Drives the M01/M02 contract certificate plumbing until the ISO
        // 15118 stack exists. Dummy base64 EXI payload.
        bool update = line[6] != '\0';
        printf("[SIM  ] forwarding an EV certificate %s request\n", update ? "update" : "install");
        if (!cp.request15118EVCertificate("urn:iso:15118:2:2013:MsgDef", update, "3q2+7w==")) {
            printf("[SIM  ] EV certificate request refused\n");
        }
    } else if (strcmp(line, "m06dump") == 0) {
        // Prints the aggregated OCSP status and the retained raw
        // responses (hex) of every SECC chain, for the M06 tests.
        for (const auto &entry : cp.cert_store.all()) {
            if (entry.group != Ocpp21::CertGroup::V2GChain && entry.group != Ocpp21::CertGroup::V2G20Chain) {
                continue;
            }
            const char *status = "?";
            switch (cp.seccChainOcspStatus(entry.id)) {
                case OcppOcspStatus21::Good:    status = "good"; break;
                case OcppOcspStatus21::Revoked: status = "revoked"; break;
                case OcppOcspStatus21::Unknown: status = "unknown"; break;
                case OcppOcspStatus21::Invalid: status = "invalid"; break;
            }
            printf("[SIM  ] m06 chain %u group %d status %s\n", entry.id, (int)entry.group, status);
            for (uint8_t idx = 0; idx < OCPP21_CHAIN_MAX_CHILDREN + 1; ++idx) {
                const uint8_t *der = nullptr;
                size_t der_len = 0;
                if (!cp.seccChainOcspResponse(entry.id, idx, &der, &der_len)) {
                    continue;
                }
                printf("[SIM  ] m06 staple chain %u idx %u ", entry.id, idx);
                for (size_t i = 0; i < der_len; ++i) {
                    printf("%02x", der[i]);
                }
                printf("\n");
            }
        }
    } else if (strcmp(line, "fault") == 0) {
        e.state = EvseState21::Faulted;
        printf("[SIM  ] EVSE faulted\n");
    } else if (strcmp(line, "ok") == 0) {
        e.state = EvseState21::NotConnected;
        printf("[SIM  ] EVSE fault cleared\n");
    } else if (line[0] != '\0') {
        printf("[SIM  ] unknown command %s (plug, unplug, detect, suspend, resume, tag <id>, stop <reason>, fault, ok, secevent <type>, m07 [count], m06dump, evcert [update])\n", line);
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
            // One command per tick, otherwise the stack could miss state
            // transitions (e.g. unplug directly followed by plug).
            return;
        }
        if (used < sizeof(line) - 1)
            line[used++] = c;
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <websocket-endpoint-url> <charge-point-name> [basic-auth-pass] [--ca file] [--cert file] [--key file] [--name2 name] [--pass2 pass]\n", argv[0]);
        fprintf(stderr, "wss:// URLs require --ca (security profile 2), --cert and --key add mTLS (security profile 3).\n");
        fprintf(stderr, "--name2 starts a second, parallel client on the same endpoint.\n");
        fprintf(stderr, "EVSE simulation via stdin: plug, unplug, detect, suspend, resume, tag <id>, stop <reason>, fault, ok, secevent <type>\n");
        return 1;
    }

    const char *pass = (argc >= 4 && strncmp(argv[3], "--", 2) != 0) ? argv[3] : nullptr;
    const char *name2 = nullptr;
    const char *pass2 = nullptr;
    PlatformTlsConfig tls;

    for (int i = pass != nullptr ? 4 : 3; i < argc - 1; ++i) {
        if (strcmp(argv[i], "--ca") == 0)
            tls.ca_cert_file = argv[++i];
        else if (strcmp(argv[i], "--cert") == 0)
            tls.client_cert_file = argv[++i];
        else if (strcmp(argv[i], "--key") == 0)
            tls.client_key_file = argv[++i];
        else if (strcmp(argv[i], "--name2") == 0)
            name2 = argv[++i];
        else if (strcmp(argv[i], "--pass2") == 0)
            pass2 = argv[++i];
        else {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            return 1;
        }
    }

    bool is_tls = strncmp(argv[1], "wss:", 4) == 0;
    int32_t security_profile = 1;
    if (is_tls)
        security_profile = tls.client_cert_file != nullptr ? 3 : 2;

    setvbuf(stdout, nullptr, _IOLBF, 0);
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    srand((unsigned)time(nullptr) ^ (unsigned)getpid());

    platform_set_system_time(nullptr, time(nullptr));

    if (!cp.start(argv[1], argv[2], pass, security_profile, is_tls ? &tls : nullptr)) {
        fprintf(stderr, "Failed to start charge point\n");
        return 1;
    }

    // Stands in for the ISO 15118 stack of the simulated station.
    snprintf(cp.device_model.protocol_supported[0], sizeof(cp.device_model.protocol_supported[0]), "urn:iso:15118:2:2013:MsgDef,2,0");
    snprintf(cp.device_model.protocol_supported[1], sizeof(cp.device_model.protocol_supported[1]), "urn:iso:std:iso:15118:-20:AC,1,0");
    cp.register15118EVCertificateResult([](bool accepted, const char *exi_response, int32_t remaining_contracts, void *user_data){
        (void)user_data;
        printf("[SIM  ] EV certificate result: %s, %d remaining, exi %s\n",
               accepted ? "Accepted" : "Failed", remaining_contracts, exi_response != nullptr ? exi_response : "-");
    }, nullptr);

    if (name2 != nullptr && !cp2.start(argv[1], name2, pass2, security_profile, is_tls ? &tls : nullptr)) {
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
