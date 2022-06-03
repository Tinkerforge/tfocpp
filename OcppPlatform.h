#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <time.h>

#include <functional>

#include "OcppMessages.h"

void *platform_init(const char *websocket_url);
void platform_disconnect(void *ctx);
void platform_destroy(void *ctx);

uint32_t platform_now_ms();
void platform_set_system_time(void *ctx, time_t t);
time_t platform_get_system_time(void *ctx);

bool platform_ws_connected(void *ctx);
void platform_ws_send(void *ctx, const char *buf, size_t buf_len);

void platform_ws_register_receive_callback(void *ctx, void(*cb)(char *, size_t, void *), void *user_data);

void platform_printfln(const char *fmt, ...) __attribute__((__format__(__printf__, 1, 2)));

void platform_register_tag_seen_callback(void *ctx, void(*cb)(const char *, void *), void *user_data);

enum TagRejectionType {
    Blocked,
    Expired,
    Invalid,
    ConcurrentTx
};

void platform_tag_rejected(const char *tag, TagRejectionType trt);

void platform_select_connector();
void platform_register_select_connector_callback(void *ctx, void(*cb)(int32_t, void *), void *user_data);

enum ConnectorState {
    NotConnected,
    Connected,
    ReadyToCharge,
    Charging,
    Faulted
};

ConnectorState platform_get_connector_state(int32_t connectorId);

/*
Value as a “Raw” (decimal) number or “SignedData”. Field Type is
“string” to allow for digitally signed data readings. Decimal numeric values are
also acceptable to allow fractional values for measurands such as Temperature
and Current.
*/
const char * platform_get_meter_value(int32_t connectorId, SampledValueMeasurand measurant);

// This is the Energy.Active.Import.Register measurant in Wh
int32_t platform_get_energy(int32_t connectorId);
