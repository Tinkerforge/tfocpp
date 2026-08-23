#pragma once

// OCPP 2.1 specific platform hooks. The shared platform API lives in
// common/Platform.h. The 21 suffix on names avoids collisions with the
// 1.6 hooks in ocpp16/Platform16.h when both stacks are linked into one
// binary.

#include <stdint.h>

enum class EvseState21 : uint8_t {
    NotConnected, // no cable plugged
    Connected,    // cable plugged, no energy flow
    Charging,     // energy flows
    Faulted,
};

// evse_id is the EVSE the tag reader belongs to, 0 for a station wide reader.
void platform_register_tag_seen_callback21(void *ctx, void (*cb)(int32_t evse_id, const char *tag_id, void *user_data), void *user_data);

EvseState21 platform_get_evse_state21(void *ctx, int32_t evse_id);

// Close (true) or open (false) the power path of the EVSE.
void platform_set_charging_allowed21(void *ctx, int32_t evse_id, bool allowed);

// Energy.Active.Import.Register of the EVSE meter in Wh.
float platform_get_energy_wh21(void *ctx, int32_t evse_id);
