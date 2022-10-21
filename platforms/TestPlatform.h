#include "ocpp/Platform.h"

extern "C" {
void set_platform_now_ms_cb(uint32_t (*cb)());
void set_platform_set_system_time_cb(void (*cb)(void *ctx, time_t t));
void set_platform_get_system_time_cb(time_t (*cb)(void *ctx));
void set_platform_printfln_cb(void (*cb)(const char *buf));
void set_platform_register_tag_seen_callback_cb(void (*cb)(void *ctx, void(*cb)(int32_t, const char *, void *), void *user_data));
void set_platform_tag_rejected_cb(void (*cb)(const char *tag, TagRejectionType trt));
void set_platform_tag_timed_out_cb(void (*cb)(int32_t connectorId));
void set_platform_cable_timed_out_cb(void (*cb)(int32_t connectorId));
void set_platform_lock_cable_cb(void (*cb)(int32_t connectorId));
void set_platform_unlock_cable_cb(void (*cb)(int32_t connectorId));
void set_platform_get_evse_state_cb(EVSEState (*cb)(int32_t connectorId));
void set_platform_set_charging_current_cb(void (*cb)(int32_t connectorId, uint32_t milliAmps));
void set_platform_get_meter_value_cb(const char * (*cb)(int32_t connectorId, SampledValueMeasurand measurant));
void set_platform_get_energy_cb(int32_t (*cb)(int32_t connectorId));
void set_platform_register_stop_callback_cb(void (*cb)(void *ctx, void (*cb)(int32_t, StopReason, void *), void *user_data));
void set_platform_reset_cb(void (*cb)());
void set_platform_read_file_cb(size_t (*cb)(const char *name, char *buf, size_t len));
void set_platform_write_file_cb(bool (*cb)(const char *name, char *buf, size_t len));

void ocpp_start(const char *ws_url, const char *charge_point_name);
void ocpp_tick();
void ocpp_stop();
void ocpp_destroy();
}
