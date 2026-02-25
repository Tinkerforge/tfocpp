#pragma once

#include <stdint.h>
#include <time.h>
#include "Connection.h"
#include "ChargingProfile.h"

#define OCPP_MAX_PERSISTENT_MESSAGES 100

// filenames are [int64_t message_id].[call_action_string]
// len(INT64_MAX) = 20 + 1 for minus + 1 for dot
// NOT NULL TERMINATED
static constexpr size_t NAME_LENGTH = 20 + 3 + CallActionStringsMaxLength;

// directory is either txn_msg/ or txn_log/
// NOT NULL TERMINATED
static constexpr size_t DIR_LENGTH = 8;

// + 1 for null terminator
static constexpr size_t PATH_BUF_SIZE = DIR_LENGTH + NAME_LENGTH + 1;

enum class TxnLogTag: uint8_t {
    BEGIN, // ExtOCMF
    SIGNED_MV_START, // OCMF container
    UNSIGNED_MV_START, // top-level int in Wh
    RECOVERY_STOPPING_EMPTY_TXN,
    START_TXN_REQ, // StartTxn.Req
    TXN_ID, // top-level int
    MV_START_REQ, // MeterValues.req containing the signed_mv_start container
    MV_START_CONFIRMED, // top-level null
    SIGNED_MV_STOP, // OCMF container
    UNSIGNED_MV_STOP, // top-level int in Wh
    STOP_TXN_REQ, // StopTxn.Req containing unsigned and optionally signed meter value
    max_ = STOP_TXN_REQ
};

void txnLogAppend(int32_t connector_id, TxnLogTag tag, const ICall &message);
void txnLogAppend(int32_t connector_id, TxnLogTag tag, const char *json, size_t json_len);
std::unique_ptr<char[]> txnLogRestore(int32_t connector_id, size_t *out_length);
void txnLogRemove(int32_t connector_id);

struct RestoredMessage : public ICall {
    std::unique_ptr<char[]> buf;
    size_t len;

    RestoredMessage(CallAction call_action, char *buffer);

    size_t serializeJson(char *buffer, size_t buf_len) const override;
};

/*
struct PersistedFile {
    uint64_t message_id;
    size_t size;
    CallAction action;
    char path[PATH_BUF_SIZE];
};



void persistMessage(const ICall &message);
void onTxnMsgResponseReceived(uint64_t call_id);

void initRestore();
bool shouldRestore();
bool restoreNextTxnMessage(OcppConnection *conn);
void finishRestore();
*/

void persistChargingProfile(int32_t connectorId, ChargingProfile *profile);
void restoreChargingProfile(int32_t connectorId, ChargingProfilePurpose purpose, int32_t stackLevel, Option<ChargingProfile> *profile);
void removeChargingProfile(int32_t connectorId, ChargingProfile *profile);
void removeChargingProfile(int32_t connectorId, ChargingProfilePurpose purpose, int32_t stackLevel);
