#pragma once

#include <memory>

#include "Messages.h"
#include "Platform.h"

namespace SMVH {
std::unique_ptr<char[]> serializeSignedMeterValue(
        const char *public_key,
        ExtSMVSignedMeterValueTypeEncodingMethod encoding_method,
        ExtSMVSignedMeterValueTypeSigningMethod signing_method,
        const char *signedMeterData
    );

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
        MeterValue &out_mv);

struct ParseResult {
    ParseResult(DynamicJsonDocument &&doc_);
    ParseResult() = default;
    DynamicJsonDocument doc{0};
    ExtOCMFView get_view() { return ExtOCMFView{doc.as<JsonObject>()}; }
};

Option<ParseResult> parseOCMFPayload(const char *data);

Option<ParseResult> parseOCMF(const char *data, size_t data_len);

Option<int32_t> getEnergyWhFromOCMF(ExtOCMFView ocmf, size_t reading);

Option<time_t> getReadingTimestampFromOCMF(ExtOCMFView ocmf, size_t reading);

MeterValues buildTxnStartMeterValues(
        int32_t connector_id,
        int32_t transaction_id,
        time_t timestamp,
        ExtSMVSignedMeterValueTypeEncodingMethod encoding_method,
        ExtSMVSignedMeterValueTypeSigningMethod signing_method,
        const char *signedMeterData,
        std::unique_ptr<char[]> &out_signed_meter_value,
        MeterValueSampledValue &out_sv,
        MeterValue &out_mv);
};
