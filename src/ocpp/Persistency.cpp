#include "Persistency.h"

#include "Types.h"
#include "Platform.h"
#include "ChargePoint.h"

#include <string.h>

#include <algorithm>
#include <functional>
#include <vector>

#define PERSISTENT_TYPE_START_TXN 1
#define PERSISTENT_TYPE_STOP_TXN 2
#define PERSISTENT_TYPE_METER_VALUES 3
#define PERSISTENT_TYPE_RUNNING_TXN 4

struct StartTxn {
    uint8_t type = PERSISTENT_TYPE_START_TXN;
    int32_t connector_id;
    int32_t meter_start;
    int32_t reservation_id;
    char id_tag[21];
    time_t timestamp;
} __attribute__((__packed__));

struct RunningTxn {
    uint8_t type = PERSISTENT_TYPE_RUNNING_TXN;
    int32_t connector_id;
    int32_t transaction_id;
} __attribute__((__packed__));

struct StopTxn {
    uint8_t type = PERSISTENT_TYPE_STOP_TXN;
    uint8_t reason;
    int32_t meter_stop;
    int32_t transaction_id;
    char id_tag[21];
    time_t timestamp;
} __attribute__((__packed__));

static_assert(((size_t)StopTransactionReason::NONE) < 255, "");

#define NAME_BUF_SIZE 21 //len(INT64_MAX) = 20 + 1 for minus + for null terminator

void persistStartTxn(int32_t connector_id, const char id_tag[21], int32_t meter_start, int32_t reservation_id, uint64_t call_id, time_t timestamp)
{
    log_info("Persisting StartTransaction.req at connector %d for tag %s at %.3f kWh as txn_msg/%llu.", connector_id, id_tag, meter_start / 1000.0f, call_id);
    StartTxn txn;
    txn.connector_id = connector_id;
    txn.meter_start = meter_start;
    txn.reservation_id = reservation_id;
    txn.timestamp = timestamp;
    memset(txn.id_tag, 0, sizeof(txn.id_tag));
    // - 1 is not necessary here, because id_tag should be null terminated.
    // But it does not hurt to do this correctly.
    strncpy(txn.id_tag, id_tag, ARRAY_SIZE(txn.id_tag) - 1);

    char buf[sizeof(StartTxn)] = {0};
    memcpy(buf, &txn, sizeof(StartTxn));

    constexpr size_t bufsize = NAME_BUF_SIZE + 8;
    char name_buf[bufsize] = "txn_msg/";
    snprintf(name_buf + 8, sizeof(name_buf) - 8, "%llu", call_id);

    platform_write_file(name_buf, buf, sizeof(StartTxn));
}

void persistRunningTxn(int32_t connector_id, uint64_t call_id, int32_t transaction_id)
{
    log_info("Persisting running transaction %d at connector %d as txn_msg/%llu.", transaction_id, connector_id, call_id);
    RunningTxn txn;
    txn.connector_id = connector_id;
    txn.transaction_id = transaction_id;

    char buf[sizeof(RunningTxn)] = {0};
    memcpy(buf, &txn, sizeof(RunningTxn));

    constexpr size_t bufsize = NAME_BUF_SIZE + 8;
    char name_buf[bufsize] = "txn_msg/";
    snprintf(name_buf + 8, sizeof(name_buf) - 8, "%llu", call_id);

    platform_write_file(name_buf, buf, sizeof(RunningTxn));
}

void persistStopTxn(uint8_t reason, int32_t meter_stop, int32_t transaction_id, const char id_tag[21], uint64_t call_id, time_t timestamp)
{
    log_info("Persisting StopTransaction.req for tag %s at %.3f kWh with StopReason %d as txn_msg/%llu", id_tag, meter_stop / 1000.0f, reason, call_id);
    StopTxn txn;
    txn.reason = reason;
    txn.meter_stop = meter_stop;
    txn.transaction_id = transaction_id;
    txn.timestamp = timestamp;
    memset(txn.id_tag, 0, sizeof(txn.id_tag));
    if (id_tag != nullptr) {
        // - 1 is not necessary here, because id_tag should be null terminated.
        // But it does not hurt to do this correctly.
        strncpy(txn.id_tag, id_tag, ARRAY_SIZE(txn.id_tag) - 1);
    }

    char buf[sizeof(StopTxn)] = {0};
    memcpy(buf, &txn, sizeof(StopTxn));

    constexpr size_t bufsize = NAME_BUF_SIZE + 8;
    char name_buf[bufsize] = "txn_msg/";
    snprintf(name_buf + 8, sizeof(name_buf) - 8, "%llu", call_id);

    platform_write_file(name_buf, buf, sizeof(StopTxn));
}

void onTxnMsgResponseReceived(uint64_t call_id)
{
    log_info("Will remove persisted message txn_msg/%llu if it exists", call_id);
    constexpr size_t bufsize = NAME_BUF_SIZE + 8;
    char name_buf[bufsize] = "txn_msg/";
    snprintf(name_buf + 8, sizeof(name_buf) - 8, "%llu", call_id);
    platform_remove_file(name_buf);
}

static std::unique_ptr<std::vector<uint64_t>> names;

void initRestore() {
    auto dir_fd = platform_open_dir("txn_msg");
    if (dir_fd == nullptr)
        return;

    names = std::unique_ptr<std::vector<uint64_t>>(new std::vector<uint64_t>());
    names->reserve(OCPP_MAX_PERSISTENT_MESSAGES);

    OcppDirEnt *dirent = nullptr;
    while ((dirent = platform_read_dir(dir_fd)) != nullptr) {
        if (dirent->is_dir)
            continue;

        if (dirent->name[0] == '\0')
            continue;

        char *endptr = nullptr;
        auto x = strtoull(dirent->name, &endptr, 10);
        if (*endptr != '\0' || x <= 0) {
            platform_remove_file(dirent->name);
            continue;
        }

        names->push_back(x);
    }

    platform_close_dir(dir_fd);

    names->shrink_to_fit();

    if (names->size() == 0)
        return;

    // sort descending: we can then always take the last element
    // and use push_back to remove it without memmoving the whole vector
    std::sort(names->begin(), names->end(), std::greater<uint64_t>());

    // Start new call IDs one behind the last persisted call.
    // This makes sure that we can reuse all persisted call IDs.
    next_call_id = names->back() + 1;

    return;
}

bool shouldRestore() {
    return names->size() > 0;
}

void finishRestore() {
    names.reset();
}

bool restoreNextTxnMessage(OcppConnection *conn) {
    while (names->size() > 0) {
        uint64_t call_id = (*names)[names->size() - 1];
        names->pop_back();

        constexpr size_t bufsize = NAME_BUF_SIZE + 8;
        char name_buf[bufsize] = "txn_msg/";
        snprintf(name_buf + 8, sizeof(name_buf) - 8, "%llu", call_id);

        char buf[100] = {0};

        auto len = platform_read_file(name_buf, buf, sizeof(buf));
        if (len < 1) {
            platform_remove_file(name_buf);
            continue;
        }

        switch (buf[0]) {
            case PERSISTENT_TYPE_START_TXN: {
                    if (len != sizeof(StartTxn))
                        break;

                    StartTxn start_txn;
                    memcpy(&start_txn, buf, sizeof(StartTxn));
                    log_info("Creating RESTORED StartTransaction.req at connector %d for tag %s at %.3f kWh.", start_txn.connector_id, start_txn.id_tag, start_txn.meter_start / 1000.0f);

                    StartTransaction msg{start_txn.connector_id, start_txn.id_tag, start_txn.meter_start, start_txn.timestamp, start_txn.reservation_id};
                    msg.ocppJmessageId = call_id;

                    conn->sendCallAction(msg, start_txn.connector_id);
                    return true;
                }
            case PERSISTENT_TYPE_STOP_TXN: {
                    if (len != sizeof(StopTxn))
                        break;

                    StopTxn stop_txn;
                    memcpy(&stop_txn, buf, sizeof(StopTxn));
                    log_info("Creating RESTORED StopTransaction.req for tag %s at %.3f kWh. Reason %d", stop_txn.id_tag, stop_txn.meter_stop / 1000.0f, stop_txn.reason);

                    StopTransaction msg{stop_txn.meter_stop, stop_txn.timestamp, stop_txn.transaction_id, stop_txn.id_tag[0] == '\0' ? nullptr : stop_txn.id_tag, (StopTransactionReason)stop_txn.reason};
                    msg.ocppJmessageId = call_id;

                    conn->sendCallAction(msg);
                    return true;
                }
            case PERSISTENT_TYPE_RUNNING_TXN: {
                if (len != sizeof(RunningTxn))
                    break;

                RunningTxn txn;
                memcpy(&txn, buf, sizeof(RunningTxn));

                auto new_timestamp = platform_get_system_time(conn->cp->platform_ctx);
                auto new_energy = platform_get_energy(txn.connector_id);

                platform_remove_file(name_buf);

                StopTransaction msg{new_energy, new_timestamp, txn.transaction_id, nullptr, StopTransactionReason::REBOOT};
                persistStopTxn((uint8_t)StopTransactionReason::REBOOT, new_energy, txn.transaction_id, "", msg.ocppJmessageId, new_timestamp);
                log_info("Creating RESTORED StopTransaction.req at connector %d for unknown tag at %.3f kWh. (Txn stopped on reboot)", txn.connector_id, new_energy / 1000.0f);
                conn->sendCallAction(msg, txn.connector_id);
                return true;
            }
            case PERSISTENT_TYPE_METER_VALUES:
            default:
                break;
        }
        platform_remove_file(name_buf);

    }
    return false;
}


void persistChargingProfile(int32_t connectorId, ChargingProfile *profile) {
    char name_buf[64] = {0};
    snprintf(name_buf, sizeof(name_buf), "profiles/%d-%d-%d", connectorId, (int32_t)profile->chargingProfilePurpose, profile->stackLevel);

    char buf[sizeof(ChargingProfile)] = {0};
    memcpy(buf, profile, sizeof(ChargingProfile));

    platform_write_file(name_buf, buf, sizeof(buf));
}

void restoreChargingProfile(int32_t connectorId, ChargingProfilePurpose purpose, int32_t stackLevel, Opt<ChargingProfile> *profile) {
    char name_buf[64] = {0};
    snprintf(name_buf, sizeof(name_buf), "profiles/%d-%d-%d", connectorId, (int32_t)purpose, stackLevel);

    char buf[sizeof(ChargingProfile)] = {0};
    auto len = platform_read_file(name_buf, buf, sizeof(ChargingProfile));
    if (len != sizeof(ChargingProfile)) {
        platform_remove_file(name_buf);
        profile->clear();
        return;
    }

    ChargingProfile cpy;
    memcpy(&cpy, buf, sizeof(cpy));

    *profile = {cpy};
}

void removeChargingProfile(int32_t connectorId, ChargingProfile *profile) {
    removeChargingProfile(connectorId, profile->chargingProfilePurpose, profile->stackLevel);
}

void removeChargingProfile(int32_t connectorId, ChargingProfilePurpose purpose, int32_t stackLevel) {
    char name_buf[64] = {0};
    snprintf(name_buf, sizeof(name_buf), "profiles/%d-%d-%d", connectorId, (int32_t)purpose, stackLevel);

    platform_remove_file(name_buf);
}
