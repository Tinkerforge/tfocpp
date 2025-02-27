#ifdef OCPP_PLATFORM_TEST

#include "TestPlatform.h"

#include "LinuxWS.h"

#include "ocpp/ChargePoint.h"
#include "time.h"

#define TFJSON_IMPLEMENTATION
#include "TFJson.h"

static uint32_t (*platform_now_ms_cb)() = nullptr;
static void (*platform_set_system_time_cb)(void *ctx, time_t t) = nullptr;
static time_t (*platform_get_system_time_cb)(void *ctx) = nullptr;

static void (*platform_printfln_cb)(const char *buf) = nullptr;

static void (*platform_register_tag_seen_callback_cb)(void *ctx, void(*cb)(int32_t, const char *, void *), void *user_data) = nullptr;
static void (*platform_register_stop_callback_cb)(void *ctx, void (*cb)(int32_t, StopReason, void *), void *user_data) = nullptr;

static void (*platform_tag_rejected_cb)(const char *tag, TagRejectionType trt) = nullptr;
static void (*platform_tag_timed_out_cb)(int32_t connectorId) = nullptr;
static void (*platform_cable_timed_out_cb)(int32_t connectorId) = nullptr;
static void (*platform_lock_cable_cb)(int32_t connectorId) = nullptr;
static void (*platform_unlock_cable_cb)(int32_t connectorId) = nullptr;
static void (*platform_set_charging_current_cb)(int32_t connectorId, uint32_t milliAmps) = nullptr;

static EVSEState (*platform_get_evse_state_cb)(int32_t connectorId) = nullptr;

static const char *(*platform_get_meter_value_cb)(int32_t connectorId, SampledValueMeasurand measurant) = nullptr;

static int32_t (*platform_get_energy_cb)(int32_t connectorId) = nullptr;

static void (*platform_reset_cb)(bool hard) = nullptr;

static size_t (*platform_read_file_cb)(const char *name, char *buf, size_t len) = nullptr;
static bool (*platform_write_file_cb)(const char *name, char *buf, size_t len) = nullptr;

static size_t (*platform_get_supported_measurand_count_cb)(int32_t connector_id, SampledValueMeasurand measurand) = nullptr;
static SupportedMeasurand *(*platform_get_supported_measurands_cb)(int32_t connector_id, SampledValueMeasurand measurand) = nullptr;

static float (*platform_get_raw_meter_value_cb)(int32_t connectorId, SampledValueMeasurand measurant, SampledValuePhase phase, SampledValueLocation location) = nullptr;

void set_platform_now_ms_cb(uint32_t (*cb)()) {platform_now_ms_cb = cb;}
void set_platform_set_system_time_cb(void (*cb)(void *ctx, time_t t)) {platform_set_system_time_cb = cb;}
void set_platform_get_system_time_cb(time_t (*cb)(void *ctx)) {platform_get_system_time_cb = cb;}
void set_platform_printfln_cb(void (*cb)(const char *buf)) {platform_printfln_cb = cb;}
void set_platform_register_tag_seen_callback_cb(void (*cb)(void *ctx, void(*cb)(int32_t, const char *, void *), void *user_data)) {platform_register_tag_seen_callback_cb = cb;}
void set_platform_tag_rejected_cb(void (*cb)(const char *tag, TagRejectionType trt)) {platform_tag_rejected_cb = cb;}
void set_platform_tag_timed_out_cb(void (*cb)(int32_t connectorId)) {platform_tag_timed_out_cb = cb;}
void set_platform_cable_timed_out_cb(void (*cb)(int32_t connectorId)) {platform_cable_timed_out_cb = cb;}
void set_platform_lock_cable_cb(void (*cb)(int32_t connectorId)) {platform_lock_cable_cb = cb;}
void set_platform_unlock_cable_cb(void (*cb)(int32_t connectorId)) {platform_unlock_cable_cb = cb;}
void set_platform_get_evse_state_cb(EVSEState (*cb)(int32_t connectorId)) {platform_get_evse_state_cb = cb;}
void set_platform_set_charging_current_cb(void (*cb)(int32_t connectorId, uint32_t milliAmps)) {platform_set_charging_current_cb = cb;}
void set_platform_get_meter_value_cb(const char * (*cb)(int32_t connectorId, SampledValueMeasurand measurant)) {platform_get_meter_value_cb = cb;}
void set_platform_get_energy_cb(int32_t (*cb)(int32_t connectorId)) {platform_get_energy_cb = cb;}
void set_platform_register_stop_callback_cb(void (*cb)(void *ctx, void (*cb)(int32_t, StopReason, void *), void *user_data)) {platform_register_stop_callback_cb = cb;}
void set_platform_reset_cb(void (*cb)(bool)) {platform_reset_cb = cb;}

void set_platform_read_file_cb(size_t (*cb)(const char *name, char *buf, size_t len)) { platform_read_file_cb = cb;}
void set_platform_write_file_cb(bool (*cb)(const char *name, char *buf, size_t len)) { platform_write_file_cb = cb;}

void set_platform_get_supported_measurand_count_cb(size_t (*cb)(int32_t connector_id, SampledValueMeasurand measurand)) {platform_get_supported_measurand_count_cb = cb;}
void set_platform_get_supported_measurands_cb(SupportedMeasurand *(*cb)(int32_t connector_id, SampledValueMeasurand measurand)) {platform_get_supported_measurands_cb = cb;}

void set_platform_get_raw_meter_value_cb(float (*cb)(int32_t connectorId, SampledValueMeasurand measurant, SampledValuePhase phase, SampledValueLocation location)) { platform_get_raw_meter_value_cb = cb;}

//------------------------------

uint32_t platform_now_ms() {
    return platform_now_ms_cb();
}

void platform_set_system_time(void *ctx, time_t t) {
    return platform_set_system_time_cb(ctx, t);
}

time_t platform_get_system_time(void *ctx) {
    return platform_get_system_time_cb(ctx);
}

void platform_printfln(const char *fmt, ...) {
    char buf[1024] = {0};
    va_list lst;
    va_start(lst, fmt);
    vsnprintf(buf, 1024, fmt, lst);
    return platform_printfln_cb(buf);
}

void platform_register_tag_seen_callback(void *ctx, void(*cb)(int32_t, const char *, void *), void *user_data) {
    return platform_register_tag_seen_callback_cb(ctx, cb, user_data);
}

void platform_tag_rejected(const char *tag, TagRejectionType trt) {
    return platform_tag_rejected_cb(tag, trt);
}

void platform_tag_timed_out(int32_t connectorId) {
    return platform_tag_timed_out_cb(connectorId);
}

void platform_cable_timed_out(int32_t connectorId) {
    return platform_cable_timed_out_cb(connectorId);
}

void platform_lock_cable(int32_t connectorId) {
    return platform_lock_cable_cb(connectorId);
}

void platform_unlock_cable(int32_t connectorId) {
    return platform_unlock_cable_cb(connectorId);
}

void platform_set_charging_current(int32_t connectorId, uint32_t milliAmps) {
    return platform_set_charging_current_cb(connectorId, milliAmps);
}

EVSEState platform_get_evse_state(int32_t connectorId) {
    return platform_get_evse_state_cb(connectorId);
}

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
const char *platform_get_meter_value(int32_t connectorId, SampledValueMeasurand measurant, SampledValuePhase phase, SampledValueLocation location, bool *is_signed) {
    (void)phase;
    (void)location;
    (void)is_signed;
    return platform_get_meter_value_cb(connectorId, measurant);
}

// This is the Energy.Active.Import.Register measurant in Wh
int32_t platform_get_energy(int32_t connectorId) {
    return platform_get_energy_cb(connectorId);
}

void platform_register_stop_callback(void *ctx, void (*cb)(int32_t, StopReason, void *), void *user_data) {
    return platform_register_stop_callback_cb(ctx, cb, user_data);
}

static OcppChargePoint *cp = nullptr;

void ocpp_start(const char *ws_url, const char *charge_point_name)
{
    cp = new OcppChargePoint();
    cp->start(ws_url, charge_point_name, nullptr, 0, /*FIXME*/ BasicAuthPassType::TEXT);
}

void ocpp_tick()
{
    ws_tick();
    cp->tick();
}

void ocpp_stop() {
    cp->stop();
}

void ocpp_destroy() {
    platform_destroy(nullptr);
    delete cp;
    cp = nullptr;
}

void platform_reset(bool hard) {
    platform_reset_cb(hard);
}

size_t platform_read_file(const char *name, char *buf, size_t len) {
    return platform_read_file_cb(name, buf, len);
}
bool platform_write_file(const char *name, char *buf, size_t len) {
    return platform_write_file_cb(name, buf, len);
}

size_t platform_get_supported_measurand_count(int32_t connector_id, SampledValueMeasurand measurand) {
    return platform_get_supported_measurand_count_cb(connector_id, measurand);
}
const SupportedMeasurand *platform_get_supported_measurands(int32_t connector_id, SampledValueMeasurand measurand) {
    return platform_get_supported_measurands_cb(connector_id, measurand);
}

bool platform_get_signed_meter_value(int32_t connectorId, SampledValueMeasurand measurant, SampledValuePhase phase, SampledValueLocation location, char buf[OCPP_PLATFORM_MEASURAND_MAX_DATA_LEN]) {
    (void)connectorId;
    (void)measurant;
    (void)phase;
    (void)location;
    (void)buf;
    //return platform_get_signed_meter_value_cb(connectorId, measurant, phase, location, buf);
    return false;
}

float platform_get_raw_meter_value(int32_t connectorId, SampledValueMeasurand measurant, SampledValuePhase phase, SampledValueLocation location) {
    return platform_get_raw_meter_value_cb(connectorId, measurant, phase, location);
}


// return nullptr if name does not exist or is not a directory
void *platform_open_dir(const char *name){
    (void)name;
    return nullptr;
}

// return nullptr if no more files
OcppDirEnt *platform_read_dir(void *dir_fd){
    (void)dir_fd;
    return nullptr;
}
void platform_close_dir(void *dir_fd){
    (void)dir_fd;
    return;
}

void platform_remove_file(const char *name){
    (void)name;
    return;
}

#endif
