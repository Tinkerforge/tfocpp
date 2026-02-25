#include "SignedMeterValueHelper.h"

namespace SMVH {
std::unique_ptr<char[]> serializeSignedMeterValue(
        const char *public_key,
        ExtSMVSignedMeterValueTypeEncodingMethod encoding_method,
        ExtSMVSignedMeterValueTypeSigningMethod signing_method,
        const char *signedMeterData
    )
{
    ExtSMVSignedMeterValueType smvt;
    smvt.encodingMethod = encoding_method;
    smvt.signingMethod = signing_method;

    smvt.publicKey = public_key == nullptr ? "" : public_key;

    smvt.signedMeterData = signedMeterData;

    size_t required; {
        TFJsonSerializer json{nullptr, 0};
        json.addObject();
        smvt.serializeInto(json);
        json.endObject();
        required = json.end();
    }

    auto meter_value_buf = heap_alloc_array<char>(required + 1);
    TFJsonSerializer json{meter_value_buf.get(), required + 1};
    json.addObject();
    smvt.serializeInto(json);
    json.endObject();
    json.end();

    return meter_value_buf;
}


StopTransaction buildStopTxn(
        int32_t connector_id,
        time_t transaction_stop_time,
        int energy,
        int32_t transaction_id,
        const char *tag_id,
        StopTransactionReason reason,
        ExtSMVSignedMeterValueTypeEncodingMethod encoding_method,
        ExtSMVSignedMeterValueTypeSigningMethod signing_method,
        const char *signedMeterData,
        std::unique_ptr<char[]> &out_signed_meter_value,
        MeterValueSampledValue &out_sv,
        MeterValue &out_mv)
{
    StopTransaction msg{energy, transaction_stop_time, transaction_id, tag_id, reason};

    log_info("Created StopTransaction.req at connector %" PRId32 " for tag %s at %.3f kWh. StopReason %d", connector_id, msg.idTag == nullptr ? "no tag" : msg.idTag, msg.meterStop / 1000.0f, (int)msg.reason);

    if (signedMeterData != nullptr) {
        bool send_public_key = false;
        auto cfg_len = getCSLConfigLen(ConfigKey::PublicKeyWithSignedMeterValue);
        if (cfg_len != 1) {
            log_error("Unexpected length of PublicKeyWithSignedMeterValue. Should be 1, is %zu", cfg_len);
        } else if (getCSLConfig(ConfigKey::PublicKeyWithSignedMeterValue)[0] != (size_t)PublicKeyWithSignedMeterValue::Never) {
            // If we shall only send the public key once, send it at the end of the transaction, i.e. now.
            send_public_key = true;
        }

        const char *publicKey = nullptr;
        if (send_public_key) {
            publicKey = getStringConfig((ConfigKey)((size_t)ConfigKey::MeterPublicKey1 + (connector_id - 1)));
        }

        out_signed_meter_value = serializeSignedMeterValue(publicKey, encoding_method, signing_method, signedMeterData);

        out_sv.context = SampledValueContext::TRANSACTION_END;
        out_sv.location = SampledValueLocation::OUTLET;
        out_sv.measurand = SampledValueMeasurand::ENERGY_ACTIVE_IMPORT_REGISTER;
        out_sv.phase = SampledValuePhase::NONE_;
        out_sv.unit = SampledValueUnit::K_WH; // TODO: should be reported by platform
        out_sv.value = out_signed_meter_value.get();
        out_sv.format = SampledValueFormat::SIGNED_DATA;

        out_mv.timestamp = transaction_stop_time;
        out_mv.sampledValue_length = 1;
        out_mv.sampledValue = &out_sv;

        msg.transactionData = &out_mv;
        msg.transactionData_length = 1;
    }

    return msg;
}

ParseResult::ParseResult(DynamicJsonDocument &&doc_) : doc(std::move(doc_)) {}

Option<ParseResult> parseOCMFPayload(const char *data) {
    // ESP32Platform's ocmf_buffer_size rounded up. (asserted there to be less than this value)
    DynamicJsonDocument doc{1536};
    auto error = deserializeJson(doc, data);
    if (error != DeserializationError::Ok) {
        log_error("Failed to parse OCMF container payload! %s", error.c_str());
        return {};
    }

    auto res = parseExtOCMF(doc.as<JsonObject>());
    if (res.result != CallErrorCode::OK) {
        log_error("Failed to parse OCMF container payload! %s: %s", CallErrorCodeStrings[(size_t)res.result], res.error_description);
        return {};
    }

    ParseResult pr{std::move(doc)};

    return {std::move(pr)};
}

Option<ParseResult> parseOCMF(const char *data, size_t data_len) {
    if (data_len < 5 || memcmp(data, "OCMF|", 5) != 0) {
        log_error("OCMF malformed! Expected container to start with \"OCMF|\"");
        return {};
    }

    return parseOCMFPayload(data + 5);
}

Option<int32_t> getEnergyWhFromOCMF(ExtOCMFView ocmf, size_t reading) {
    if (ocmf.RD_count() <= reading)
        return {};

    return (int32_t)(ocmf.RD(reading).RV() * (ocmf.RD(reading).RU() == ExtOCMFRDEntryEntriesRU::K_WH ? 1000.0f : 1.0f));
}

Option<time_t> getReadingTimestampFromOCMF(ExtOCMFView ocmf, size_t reading) {
    if (ocmf.RD_count() <= reading)
        return {};

    // OCMF iso strings are always in the format 2018-07-24T13:22:04,000+0200 U
    // where " U" is the sync status
    char buf[31];
    strncpy(buf, ocmf.RD(reading).TM(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';

    if (buf[28] != ' ') {
        log_error("Unexpected format of reading timestamp. Expected space between iso string and sync status");
        return {};
    }
    buf[28] = '\0';

    time_t result;
    if (!iso_string_to_unix_timestamp(buf, &result)) {
        log_error("Failed to parse reading timestamp");
        return {};
    }
    return {result};
}

MeterValues buildTxnStartMeterValues(
        int32_t connector_id,
        int32_t transaction_id,
        time_t timestamp,
        ExtSMVSignedMeterValueTypeEncodingMethod encoding_method,
        ExtSMVSignedMeterValueTypeSigningMethod signing_method,
        const char *signedMeterData,
        std::unique_ptr<char[]> &out_signed_meter_value,
        MeterValueSampledValue &out_sv,
        MeterValue &out_mv)
{

    bool send_public_key = false;
    auto cfg_len = getCSLConfigLen(ConfigKey::PublicKeyWithSignedMeterValue);
    if (cfg_len != 1) {
        log_error("Unexpected length of PublicKeyWithSignedMeterValue. Should be 1, is %zu", cfg_len);
    } else if (getCSLConfig(ConfigKey::PublicKeyWithSignedMeterValue)[0] == (size_t)PublicKeyWithSignedMeterValue::EveryMeterValue) {
        // If we shall only send the public key once, send it at the end of the transaction, not now.
        send_public_key = true;
    }

    const char *publicKey = nullptr;
    if (send_public_key) {
        static_assert(OCPP_NUM_CONNECTORS == 1, "only one connector is supported for now: each connector requires a separate MeterPublicKey[ConnectorID] ConfigKey");
        publicKey = getStringConfig(ConfigKey::MeterPublicKey1);
    }

    out_signed_meter_value = serializeSignedMeterValue(publicKey, encoding_method, signing_method, signedMeterData);

    out_sv.context = SampledValueContext::TRANSACTION_BEGIN;
    out_sv.location = SampledValueLocation::OUTLET;
    out_sv.measurand = SampledValueMeasurand::ENERGY_ACTIVE_IMPORT_REGISTER;
    out_sv.phase = SampledValuePhase::NONE_;
    out_sv.unit = SampledValueUnit::K_WH; // TODO: should be reported by platform
    out_sv.value = out_signed_meter_value.get();

    out_sv.format = SampledValueFormat::SIGNED_DATA;

    out_mv.timestamp = timestamp;
    out_mv.sampledValue_length = 1;
    out_mv.sampledValue = &out_sv;

    return MeterValues{connector_id, &out_mv, 1, transaction_id};
}
}
