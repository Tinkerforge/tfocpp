#include "OcppPlatform.h"

extern "C" {
uint32_t (*platform_now_ms_cb)() = NULL;
void (*platform_set_system_time_cb)(void *ctx, time_t t) = NULL;
time_t (*platform_get_system_time_cb)(void *ctx) = NULL;

void (*platform_printfln_cb)(const char *buf) = NULL;

void (*platform_register_tag_seen_callback_cb)(void *ctx, void(*cb)(int32_t, const char *, void *), void *user_data) = NULL;

void (*platform_tag_rejected_cb)(const char *tag, TagRejectionType trt) = NULL;
void (*platform_tag_timed_out_cb)(int32_t connectorId) = NULL;
void (*platform_cable_timed_out_cb)(int32_t connectorId) = NULL;
void (*platform_lock_cable_cb)(int32_t connectorId) = NULL;
void (*platform_unlock_cable_cb)(int32_t connectorId) = NULL;
void (*platform_set_charging_current_cb)(int32_t connectorId, uint32_t milliAmps) = NULL;

EVSEState (*platform_get_evse_state_cb)(int32_t connectorId) = NULL;

const char * (*platform_get_meter_value_cb)(int32_t connectorId, SampledValueMeasurand measurant) = NULL;

int32_t (*platform_get_energy_cb)(int32_t connectorId) = NULL;

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

void ocpp_start(const char *ws_url, const char *charge_point_name);
void ocpp_tick();
void ocpp_stop();
void ocpp_destroy();
}
