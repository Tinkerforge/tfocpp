#pragma once

#include <stdint.h>
#include <time.h>
#include "Connection.h"
#include "ChargingProfile.h"

#define OCPP_MAX_PERSISTENT_MESSAGES 100

void persistStartTxn(int32_t connector_id, const char id_tag[21], int32_t meter_start, int32_t reservation_id, time_t timestamp);
void persistRunningTxn(int32_t connector_id, time_t timestamp, int32_t transaction_id);
void persistStopTxn(uint8_t reason, int32_t meter_stop, int32_t transaction_id, const char id_tag[21], time_t timestamp);

void onTxnMsgResponseReceived(time_t timestamp);

bool startRestore();
bool restoreNextTxnMessage(OcppConnection *conn);
void finishRestore();

void persistChargingProfile(int32_t connectorId, ChargingProfile *profile);
void restoreChargingProfile(int32_t connectorId, ChargingProfilePurpose purpose, int32_t stackLevel, Opt<ChargingProfile> *profile);
void removeChargingProfile(int32_t connectorId, ChargingProfile *profile);
void removeChargingProfile(int32_t connectorId, ChargingProfilePurpose purpose, int32_t stackLevel);
