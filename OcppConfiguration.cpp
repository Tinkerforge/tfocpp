#include "OcppConfiguration.h"

#include "OcppDefines.h"
#include "OcppPlatform.h"
#include "OcppTools.h"

#include <string.h>

OcppConfiguration OcppConfiguration::integer(int32_t value,
                                             bool readonly,
                                             bool requires_reboot,
                                             int32_t min_,
                                             int32_t max_) {
    return OcppConfiguration{OcppConfigurationValueType::Integer, {.integer = {value,min_, max_}}, readonly, requires_reboot};
}

OcppConfiguration OcppConfiguration::boolean(bool value,
                          bool readonly,
                          bool requires_reboot) {
    return OcppConfiguration{OcppConfigurationValueType::Boolean, {.boolean = {value}}, readonly, requires_reboot};
}

OcppConfiguration OcppConfiguration::csl(const char *value,
                                 size_t max_len,
                                 size_t max_elements,
                                 bool readonly,
                                 bool requires_reboot,
                                 const char **allowed_values,
                                 size_t allowed_values_len,
                                 bool prefix_index) {
    size_t len = sizeof(char) * max_len;

    auto result = OcppConfiguration{OcppConfigurationValueType::CSL, {.csl = {(char *)malloc(len), max_len, (size_t*)malloc(sizeof(size_t) * max_elements), 0, allowed_values, allowed_values_len, prefix_index, max_elements}}, readonly, requires_reboot};

    memset(result.value.csl.c, 0, len);
    strncpy(result.value.csl.c, value, len);

    return result;
}

ChangeConfigurationResponseStatus OcppConfiguration::setValue(const char *newValue) {
    if (readonly)
        return ChangeConfigurationResponseStatus::REJECTED;

    switch (type) {
        case OcppConfigurationValueType::Integer: {
                Opt<int32_t> opt = parse_int(newValue);
                if (!opt.is_set())
                    return ChangeConfigurationResponseStatus::REJECTED;

                int32_t parsed = opt.get();

                if (!(parsed >= value.integer.min_ && parsed <= value.integer.max_))
                    return ChangeConfigurationResponseStatus::REJECTED;

                value.integer.i = parsed;
                return requires_reboot ? ChangeConfigurationResponseStatus::REBOOT_REQUIRED : ChangeConfigurationResponseStatus::ACCEPTED;
            }
            break;
        case OcppConfigurationValueType::Boolean: {
                StaticJsonDocument<10> doc;
                if (deserializeJson(doc, newValue) != DeserializationError::Ok || !doc.is<bool>())
                    return ChangeConfigurationResponseStatus::REJECTED;

                value.boolean.b = doc.as<bool>();
                return requires_reboot ? ChangeConfigurationResponseStatus::REBOOT_REQUIRED : ChangeConfigurationResponseStatus::ACCEPTED;
            }
            break;
        case OcppConfigurationValueType::CSL: {
                size_t len = strlen(newValue);

                if (len > value.csl.len)
                    return ChangeConfigurationResponseStatus::REJECTED;

                std::unique_ptr<char[]> buf{new char[value.csl.len]};
                std::unique_ptr<size_t[]> parsed_buf{new size_t[value.csl.allowed_values_len]};

                memset(buf.get(), 0, value.csl.len);
                memcpy(buf.get(), newValue, len);

                size_t next_parsed_buf_insert = 0;
                size_t new_parsed_len = 0;

                char *context;
                char *token = strtok_r(buf.get(), ",", &context);
                if (token != nullptr) {
                    do {
                        while(isspace(*token))
                            ++token;

                        if (value.csl.prefix_index) {
                            char *num = strtok(token, "."); // This insers a null terminator. undo later
                            Opt<int32_t> opt = parse_int(num);
                            if (!opt.is_set())
                                return ChangeConfigurationResponseStatus::REJECTED;

                            if (opt.get() < 0 || opt.get() >= value.csl.allowed_values_len)
                                return ChangeConfigurationResponseStatus::REJECTED;

                            next_parsed_buf_insert = opt.get();
                            token += strlen(num) + 1; // Skip over number and .
                            num[strlen(num)] = '.'; // Reinsert . so that the next strtok_r call does not trip over the null terminator.
                        }

                        size_t idx;
                        if (!lookup_key(&idx, token, value.csl.allowed_values, value.csl.allowed_values_len))
                            return ChangeConfigurationResponseStatus::REJECTED;

                        parsed_buf[next_parsed_buf_insert] = idx;
                        ++next_parsed_buf_insert; //if prefix_index is set this will be overwritten anyway.
                        ++new_parsed_len;
                    } while ((token = strtok_r(nullptr, ",", &context)) != nullptr);
                }

                if (new_parsed_len > value.csl.max_num_allowed_values)
                    return ChangeConfigurationResponseStatus::REJECTED;

                // Reinsert , removed by outer strtok_r calls.
                for(size_t i = 0; i < len; ++i)
                    if (buf[i] == '\0')
                        buf[i] = ',';

                memcpy(value.csl.c, buf.get(), len);

                memcpy(value.csl.parsed, parsed_buf.get(), sizeof(size_t) * new_parsed_len);
                value.csl.parsed_len = new_parsed_len;
                return requires_reboot ? ChangeConfigurationResponseStatus::REBOOT_REQUIRED : ChangeConfigurationResponseStatus::ACCEPTED;
            }
            break;
    }
    return ChangeConfigurationResponseStatus::REJECTED;
}

const char *connectorPhaseRotationStrings[] = {
    "NotApplicable",
    "Unknown",
    "RST",
    "RTS",
    "SRT",
    "STR",
    "TRS",
    "TSR",
};

const char *config_keys[CONFIG_COUNT] {
    //"AllowOfflineTxForUnknownId",
    //"AuthorizationCacheEnabled",
    "AuthorizeRemoteTxRequests",
    //"BlinkRepeat",
    "ClockAlignedDataInterval",
    "ConnectionTimeOut",
    "ConnectorPhaseRotation",
    "ConnectorPhaseRotationMaxLength",
    "GetConfigurationMaxKeys",
    "HeartbeatInterval",
    //"LightIntensity",
    "LocalAuthorizeOffline",
    "LocalPreAuthorize",
    //"MaxEnergyOnInvalidId",
    "MeterValuesAlignedData",
    //"MeterValuesAlignedDataMaxLength",
    "MeterValuesSampledData",
    //"MeterValuesSampledDataMaxLength",
    "MeterValueSampleInterval",
    //"MinimumStatusDuration",
    "NumberOfConnectors",
    "ResetRetries",
    "StopTransactionOnEVSideDisconnect",
    "StopTransactionOnInvalidId",
    "StopTxnAlignedData",
    //"StopTxnAlignedDataMaxLength",
    "StopTxnSampledData",
    //"StopTxnSampledDataMaxLength",
    "SupportedFeatureProfiles",
    //"SupportedFeatureProfilesMaxLength",
    "TransactionMessageAttempts",
    "TransactionMessageRetryInterval",
    "UnlockConnectorOnEVSideDisconnect",
    "WebSocketPingInterval",
};

OcppConfiguration config[CONFIG_COUNT] = {
    /*AllowOfflineTxForUnknownId*/        //OcppConfiguration::boolean(false, false, false),
    /*AuthorizationCacheEnabled*/         //OcppConfiguration::boolean(false, false, false),
    /*AuthorizeRemoteTxRequests*/         OcppConfiguration::boolean(false, false, false),
    /*BlinkRepeat*/                       //OcppConfiguration::integer(3, false, false),
    /*ClockAlignedDataInterval*/          OcppConfiguration::integer(0, false, false, 0),
    /*ConnectionTimeOut*/                 OcppConfiguration::integer(60, false, false, 0),

                                          // +1 for index 0: "the phase rotation between
                                          //    the grid connection and the main energymeter"
                                          // +5 for two digits, the dot, comma and space
                                          // +1 for null-terminator
                                          // Format is "0.RST, 1.RST, 2.RTS"
    /*ConnectorPhaseRotation*/            OcppConfiguration::csl("1.Unknown", (NUM_CONNECTORS + 1) * (strlen("NotApplicable") + 5) + 1, NUM_CONNECTORS + 1, false, false, connectorPhaseRotationStrings, ARRAY_SIZE(connectorPhaseRotationStrings), true),
    /*ConnectorPhaseRotationMaxLength*/   OcppConfiguration::integer(NUM_CONNECTORS + 1, true, false, 0),

    /*GetConfigurationMaxKeys*/           OcppConfiguration::integer(40, true, false, 0),
    /*HeartbeatInterval*/                 OcppConfiguration::integer(DEFAULT_BOOT_NOTIFICATION_RESEND_INTERVAL_S, false, false, 0),
    /*LightIntensity*/                    //OcppConfiguration::integer(100, false, false, 0),
    /*LocalAuthorizeOffline*/             OcppConfiguration::boolean(false, false, false),
    /*LocalPreAuthorize*/                 OcppConfiguration::boolean(false, false, false),
    /*MaxEnergyOnInvalidId*/              //OcppConfiguration::integer(0, false, false, 0),

    // Its save to use the number of possible measurands as limit in elements,
    // because the complete list has a length of 465.
    // This also means that we don't have to implemeht the MeterValuesAlignedDataMaxLength key.
    /*MeterValuesAlignedData*/            OcppConfiguration::csl("", MAX_CONFIG_LENGTH, (size_t)SampledValueMeasurand::NONE, false, false, SampledValueMeasurandStrings, (size_t)SampledValueMeasurand::NONE),
    /*MeterValuesAlignedDataMaxLength*/   //OcppConfiguration::integer(MAX_CONFIG_LENGTH / strlen("Energy.Reactive.Import.Register,"), true, false, 0),

    // Same reasoning as with MeterValuesAlignedData.
    /*MeterValuesSampledData*/            OcppConfiguration::csl("", MAX_CONFIG_LENGTH, (size_t)SampledValueMeasurand::NONE, false, false, SampledValueMeasurandStrings, (size_t)SampledValueMeasurand::NONE),
    /*MeterValuesSampledDataMaxLength*/   //OcppConfiguration::integer(0, true, false, 0),

    /*MeterValueSampleInterval*/          OcppConfiguration::integer(0, false, false, 0),
    /*MinimumStatusDuration*/             //OcppConfiguration::integer(1, false, false, 0),
    /*NumberOfConnectors*/                OcppConfiguration::integer(1, true, false, 0),
    /*ResetRetries*/                      OcppConfiguration::integer(1, false, false, 0),
    /*StopTransactionOnEVSideDisconnect*/ OcppConfiguration::boolean(false, false, false),
    /*StopTransactionOnInvalidId*/        OcppConfiguration::boolean(false, false, false),

    // Same reasoning as with MeterValuesAlignedData.
    /*StopTxnAlignedData*/                OcppConfiguration::csl("", MAX_CONFIG_LENGTH, (size_t)SampledValueMeasurand::NONE, false, false, SampledValueMeasurandStrings, (size_t)SampledValueMeasurand::NONE),
    /*StopTxnAlignedDataMaxLength*/       //OcppConfiguration::integer(0, true, false, 0),

    // Same reasoning as with MeterValuesAlignedData.
    /*StopTxnSampledData*/                OcppConfiguration::csl("", MAX_CONFIG_LENGTH, (size_t)SampledValueMeasurand::NONE, false, false, SampledValueMeasurandStrings, (size_t)SampledValueMeasurand::NONE),
    /*StopTxnSampledDataMaxLength*/       //OcppConfiguration::integer(0, true, false, 0),

    /*SupportedFeatureProfiles*/          OcppConfiguration::csl("Core", strlen("Core") + 1, 1, true, false, nullptr, 0, false),
    /*SupportedFeatureProfilesMaxLength*/ //OcppConfiguration::integer(1, true), //errata 4.0: "This configuration key does not have to be implemented. It should not have been part of OCPP 1.6, "SupportedFeatureProfiles" is a readonly configuration key, false, 0."
    /*TransactionMessageAttempts*/        OcppConfiguration::integer(3, false, false, 0),
    /*TransactionMessageRetryInterval*/   OcppConfiguration::integer(10, false, false, 0),
    /*UnlockConnectorOnEVSideDisconnect*/ OcppConfiguration::boolean(false, false, false),
    /*WebSocketPingInterval*/             OcppConfiguration::integer(10, false, false, 0),
};

OcppConfiguration& getConfig(ConfigKey key) {
    return config[(size_t)key];
}

OcppConfiguration& getConfig(size_t key) {
    return config[key];
}

int32_t getIntConfig(ConfigKey key) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::Integer) {
        platform_printfln("Tried to read config %s (%d) as int, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::CSL ? "CSL" : "boolean");
        return -1;
    }
    return cfg.value.integer.i;
}

bool getBoolConfig(ConfigKey key) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::Boolean) {
        platform_printfln("Tried to read config %s (%d) as bool, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::CSL ? "CSL" : "integer");
        return false;
    }
    return cfg.value.boolean.b;
}

size_t getCSLConfigLen(ConfigKey key) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::CSL) {
        platform_printfln("Tried to read config %s (%d) as csl, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::Integer ? "integer" : "boolean");
        return 0;
    }
    return cfg.value.csl.parsed_len;
}

size_t *getCSLConfig(ConfigKey key) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::CSL) {
        platform_printfln("Tried to read config %s (%d) as csl, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::Integer ? "integer" : "boolean");
        return nullptr;
    }
    return cfg.value.csl.parsed;
}

bool setIntConfig(ConfigKey key, int32_t i) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::Integer) {
        platform_printfln("Tried to write config %s (%d) as int, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::CSL ? "CSL" : "boolean");
        return false;
    }

    cfg.value.integer.i = i;
    return true;
}

bool setBoolConfig(ConfigKey key, bool b) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::Boolean) {
        platform_printfln("Tried to write config %s (%d) as bool, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::CSL ? "CSL" : "integer");
        return false;
    }
    cfg.value.boolean.b = b;
    return true;
}
