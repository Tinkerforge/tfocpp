#include "Persistency.h"

#include "Types.h"
#include "Platform.h"
#include "ChargePoint.h"

#include <string.h>
#include <inttypes.h>
#include <string.h>

#include <algorithm>
#include <functional>
#include <vector>

RestoredMessage::RestoredMessage(CallAction call_action, char *buffer) : ICall(call_action, (uint32_t)0), buf(nullptr), len(strlen(buffer)) {
    buf = heap_alloc_array<char>(this->len + 1);
    memcpy(buf.get(), buffer, this->len + 1);

    TFJsonDeserializer json{2, 0, false};
    json.setStringHandler([this](char *s, size_t s_len) {
        // string handler passes unterminated strings
        char tmp[21];
        size_t l = std::min(sizeof(tmp) - 1, s_len);
        memcpy(tmp, s, l);
        tmp[l] = '\0';

        this->ocppJmessageId = strtoull(tmp, nullptr, 10);

        next_call_id = this->ocppJmessageId + 1;
        return false;
    });
    json.parse(buffer, this->len);
}

size_t RestoredMessage::serializeJson(char *buffer, size_t buf_len) const {
    if (buf_len >= this->len)
        memcpy(buffer, this->buf.get(), this->len);
    return this->len;
}
/*
void persistMessage(const ICall &message) {
    size_t buf_size = message.measureJson();
    auto buf = heap_alloc_array<char>(buf_size);
    buf_size = message.serializeJson(buf.get(), buf_size);

    char path[PATH_BUF_SIZE];
    snprintf(path, sizeof(path), "txn_msg/%" PRIu64 ".%s", message.ocppJmessageId, CallActionStrings[(size_t)message.action]);

    platform_write_file(path, buf.get(), buf_size);
}

static RestoredMessage restoreMessage(PersistedFile &file) {
    auto buf = heap_alloc_array<char>(file.size);

    // TODO check platform_read_file return value
    platform_read_file(file.path, buf.get(), file.size);

    return RestoredMessage{file.action, file.message_id, std::move(buf)};
}
*/
void txnLogAppend(int32_t connector_id, TxnLogTag tag, const ICall &message) {
    char path[PATH_BUF_SIZE];
    snprintf(path, sizeof(path), "txn_log/%d", connector_id);

    size_t buf_size = message.measureJson() + 2;
    auto buf = heap_alloc_array<char>(buf_size);
    static_assert((size_t)TxnLogTag::max_ < 26, "We use one lower-case letter to serialize each TxnLogTag, so max 26 enum values are allowed.");
    buf[0] = 'a' + (char) tag;
    message.serializeJson(buf.get() + 1, buf_size - 1);
    buf[buf_size - 1] = '\n';

    platform_append_file(path, buf.get(), buf_size);
}

void txnLogAppend(int32_t connector_id, TxnLogTag tag, const char *json, size_t json_len) {
    char path[PATH_BUF_SIZE];
    snprintf(path, sizeof(path), "txn_log/%d", connector_id);

    size_t buf_size = json_len + 2;
    auto buf = heap_alloc_array<char>(buf_size);
    static_assert((size_t)TxnLogTag::max_ < 26, "We use one lower-case letter to serialize each TxnLogTag, so max 26 enum values are allowed.");
    buf[0] = 'a' + (char) tag;
    memcpy(buf.get() + 1, json, json_len);
    buf[buf_size - 1] = '\n';

    platform_append_file(path, buf.get(), buf_size);
}

std::unique_ptr<char[]> txnLogRestore(int32_t connector_id, size_t *out_length) {
    char path[PATH_BUF_SIZE];
    snprintf(path, sizeof(path), "txn_log/%d", connector_id);

    return platform_read_file(path, out_length);
}

void txnLogRemove(int32_t connector_id) {
    char path[PATH_BUF_SIZE];
    snprintf(path, sizeof(path), "txn_log/%d", connector_id);

    platform_remove_file(path);
}
/*
void onTxnMsgResponseReceived(uint64_t call_id, CallAction action)
{
    log_info("Will remove persisted message txn_msg/%" PRIu64 ".%s if it exists.", call_id, CallActionStrings[(size_t)action]);

    char path[PATH_BUF_SIZE];
    snprintf(path, sizeof(path), "txn_msg/%" PRIu64 ".%s", call_id, CallActionStrings[(size_t)action]);

    platform_remove_file(name_buf);
}

struct RestoredData {
    std::vector<PersistedFile> msgs;
}

static std::unique_ptr<RestoredData> restored;

uint64_t initRestoreTxnMsgs(RestoredData *restored) {
    auto dir_fd = platform_open_dir("txn_msg");
    if (dir_fd == nullptr)
        return 0;

    restored->msgs.reserve(OCPP_MAX_PERSISTENT_MESSAGES);

    OcppDirEnt *dirent = nullptr;
    while ((dirent = platform_read_dir(dir_fd)) != nullptr) {
        if (dirent->is_dir)
            continue;

        if (dirent->name[0] == '\0' || dirent->size == 0)
            continue;

        char *endptr = nullptr;
        auto message_id = strtoull(dirent->name, &endptr, 10);
        if (*endptr != '.' || message_id <= 0) {
            platform_remove_file(dirent->name);
            continue;
        }
        ++endptr;

        // Invalid marker; this is our extension that is only used for serializing the call's payload, this will never be called.
        CallAction call_action = CallAction::EXT_SMV;

        for (size_t i = 0; i < ARRAY_SIZE(CallActionStrings); ++i) {
            // We don't know whether the platform implementor has null-terminated the filename correctly.
            // Read at most CallActionStringsMaxLength, so that we only read some uninitialized memory
            // if shit hits the fan.
            if (strncmp(endptr, CallActionStrings[i], CallActionStringsMaxLength) == 0) {
                call_action = (CallAction) i;
                break;
            }
        }
        if (call_action == CallAction::EXT_SMV) {
            platform_remove_file(dirent->name);
            continue;
        }

        restored->msgs.push_back({message_id, call_action, dirent->size, ""});
        snprintf(restored->msgs.back().path, sizeof(restored->msgs.back().path), "txn_msg/%s", dirent->name);
    }

    platform_close_dir(dir_fd);

    restored->msgs.shrink_to_fit();

    if (restored->msgs.size() == 0)
        return 0;

    // sort descending: we can then always take the last element
    // and use pop_back to remove it without memmoving the whole vector
    std::sort(restored->msgs.begin(), restored->msgs.end(),
              [](const PersistedFile &left, const PersistedFile &right){
                return left.message_id > right.message_id;
              } );

    return restored->msgs.front().message_id;
}

void initRestore() {
    restored = std::unique_ptr<RestoredData>(new RestoredData());

    auto highest_msg_id = initRestoreTxnMsgs(restored.get());

    // Start new call IDs one behind the last persisted call.
    // This makes sure that we can reuse all persisted call IDs.
    // This is sorted descending, so the first entry has the largest ID.
    next_call_id = highest_msg_id + 1;

    return;
}

void finishRestore() {
    if (restored == nullptr)
        return;

    restored.reset();
}

bool restoreNextTxnMessage(OcppConnection *conn) {
    if (restored == nullptr)
        return false;

    if (restored->msgs.size() > 0) {
        auto entry = restored->msgs[restored->msgs.size() - 1];
        restored->msgs.pop_back();

        auto restored_message = restoreMessage(entry);
        conn->sendCallAction(restored_message, -1);
        return true;
    }

    return false;
}

bool restoreNextRunningTxn(RunningTxn *running_txn) {
    if (restored == nullptr || running_txn == nullptr)
        return false;

    if (restored->run_txns.size() > 0) {
        *running_txn = restored->run_txns[restored->run_txns.size() - 1];
        restored->msgs.pop_back();
        return true;
    }

    return false;
}
*/
void persistChargingProfile(int32_t connectorId, ChargingProfile *profile) {
    char name_buf[64] = {0};
    snprintf(name_buf, sizeof(name_buf), "profiles/%" PRId32 "-%" PRId32 "-%" PRId32, connectorId, (int32_t)profile->chargingProfilePurpose, profile->stackLevel);

    char buf[sizeof(ChargingProfile)] = {0};
    memcpy(buf, profile, sizeof(ChargingProfile));

    platform_write_file(name_buf, buf, sizeof(buf));
}

void restoreChargingProfile(int32_t connectorId, ChargingProfilePurpose purpose, int32_t stackLevel, Option<ChargingProfile> *profile) {
    char name_buf[64] = {0};
    snprintf(name_buf, sizeof(name_buf), "profiles/%" PRId32 "-%" PRId32 "-%" PRId32, connectorId, (int32_t)purpose, stackLevel);

    size_t len;
    auto buf = platform_read_file(name_buf, &len, sizeof(ChargingProfile));
    if (len != sizeof(ChargingProfile)) {
        platform_remove_file(name_buf);
        profile->clear();
        return;
    }

    ChargingProfile cpy;
    memcpy(&cpy, buf.get(), sizeof(cpy));

    *profile = {cpy};
}

void removeChargingProfile(int32_t connectorId, ChargingProfile *profile) {
    removeChargingProfile(connectorId, profile->chargingProfilePurpose, profile->stackLevel);
}

void removeChargingProfile(int32_t connectorId, ChargingProfilePurpose purpose, int32_t stackLevel) {
    char name_buf[64] = {0};
    snprintf(name_buf, sizeof(name_buf), "profiles/%" PRId32 "-%" PRId32 "-%" PRId32 "", connectorId, (int32_t)purpose, stackLevel);

    platform_remove_file(name_buf);
}
