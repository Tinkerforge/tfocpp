#include "Configuration.h"

#include "Defines.h"
#include "Platform.h"
#include "Tools.h"

#include <string.h>

// This is all configs specified in the OCPP spec.
#define MAX_SPECIFIED_CONFIGS 46
#define MAX_CONFIG_LENGTH 501 // the spec does not specify a maximum length. however the payload of a change configuration message has a max length of 500


OcppConfiguration OcppConfiguration::integer(int32_t value,
                                             bool readonly,
                                             bool requires_reboot,
                                             int32_t min_,
                                             int32_t max_) {
    // designated initializers are a C++20 extension, so we have to do this manually.
    OcppConfiguration result;
    result.type = OcppConfigurationValueType::Integer;
    result.value.integer.i = value;
    result.value.integer.min_ = min_;
    result.value.integer.max_ = max_;
    result.readonly = readonly;
    result.requires_reboot = requires_reboot;

    return result;
}

OcppConfiguration OcppConfiguration::boolean(bool value,
                          bool readonly,
                          bool requires_reboot) {
    // designated initializers are a C++20 extension, so we have to do this manually.
    OcppConfiguration result;
    result.type = OcppConfigurationValueType::Boolean;
    result.value.boolean.b = value;
    result.readonly = readonly;
    result.requires_reboot = requires_reboot;

    return result;
}

OcppConfiguration OcppConfiguration::csl(const char *value,
                                 size_t max_len,
                                 size_t max_elements,
                                 bool readonly,
                                 bool requires_reboot,
                                 const char * const *allowed_values,
                                 size_t allowed_values_len,
                                 bool prefix_index,
                                 bool suffix_phase) {
    size_t len = sizeof(char) * max_len;

    OcppConfiguration result;
    result.type = OcppConfigurationValueType::CSL;
    result.value.csl.c = (char *)malloc(len);
    result.value.csl.len = max_len;
    result.value.csl.parsed = (size_t*)malloc(sizeof(size_t) * max_elements);
    result.value.csl.parsed_len = 0;
    if (suffix_phase)
        result.value.csl.phases = (size_t*)malloc(sizeof(size_t) * max_elements);

    result.value.csl.allowed_values = allowed_values;
    result.value.csl.allowed_values_len = allowed_values_len;
    result.value.csl.prefix_index = prefix_index;
    result.value.csl.max_num_allowed_values = max_elements;
    result.readonly = readonly;
    result.requires_reboot = requires_reboot;

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
        case OcppConfigurationValueType::Boolean: {
                StaticJsonDocument<10> doc;
                if (deserializeJson(doc, newValue) != DeserializationError::Ok || !doc.is<bool>())
                    return ChangeConfigurationResponseStatus::REJECTED;

                value.boolean.b = doc.as<bool>();
                return requires_reboot ? ChangeConfigurationResponseStatus::REBOOT_REQUIRED : ChangeConfigurationResponseStatus::ACCEPTED;
            }
        case OcppConfigurationValueType::CSL: {
                size_t len = strlen(newValue);

                if (len >= value.csl.len) // >= because we always want to null-terminate the buffer.
                    return ChangeConfigurationResponseStatus::REJECTED;

                if (len > 0 && newValue[0] == ',') // Don't allow ",SoC"
                    return ChangeConfigurationResponseStatus::REJECTED;

                if (len > 0 && newValue[len - 1] == ',') // Don't allow "SoC,"
                    return ChangeConfigurationResponseStatus::REJECTED;

                auto buf = heap_alloc_array<char>(value.csl.len);
                auto parsed_buf = heap_alloc_array<size_t>(value.csl.allowed_values_len);
                bool phase_suffix_allowed = value.csl.phases != nullptr;
                auto phases_buf = phase_suffix_allowed ? heap_alloc_array<size_t>(value.csl.allowed_values_len) : std::unique_ptr<size_t[]>();

                memset(buf.get(), 0, value.csl.len);
                memcpy(buf.get(), newValue, len);

                size_t next_parsed_buf_insert = 0;
                size_t new_parsed_len = 0;

                char *context;
                char *token = strtok_r(buf.get(), ",", &context);
                if (token != nullptr) {
                    do {
                        size_t token_len = strlen(token);
                        if ((token_len + 1) < value.csl.len && buf.get()[strlen(token) + 1] == ',') // Don't allow "abc,,def".
                            return ChangeConfigurationResponseStatus::REJECTED;

                        while(isspace(*token))
                            ++token;

                        // Prefix indices are used for ConnectorPhaseRotation.
                        /* Values are reported in CSL, formatted: 0.RST, 1.RST, 2.RTS */
                        if (value.csl.prefix_index) {
                            char *num = strtok(token, "."); // This inserts a null terminator. undo later
                            Opt<int32_t> opt = parse_int(num);
                            if (!opt.is_set())
                                return ChangeConfigurationResponseStatus::REJECTED;

                            if (opt.get() < 0 || (size_t)opt.get() >= value.csl.allowed_values_len)
                                return ChangeConfigurationResponseStatus::REJECTED;

                            next_parsed_buf_insert = (size_t)opt.get();
                            token += strlen(num) + 1; // Skip over number and .
                            num[strlen(num)] = '.'; // Reinsert . so that the next strtok_r call does not trip over the null terminator.
                        }

                        // Suffix phases are used for MeterValuesSampledData:
                        /* Where applicable, the Measurand is combined with the optional phase; for instance: Voltage.L1 */
                        if (phase_suffix_allowed) {
                            char *last_dot = strrchr(token, '.');
                            if (last_dot != nullptr) {
                                size_t phase_idx = 0;
                                if (lookup_key(&phase_idx, last_dot + 1, SampledValuePhaseStrings, (size_t) SampledValuePhase::NONE)) {
                                    *last_dot = '\0'; // terminate string here so that lookup of the allowed value below does ignore the phase
                                    phases_buf[next_parsed_buf_insert] = phase_idx;
                                }
                            }
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

                buf[len] = '\0';

                memcpy(value.csl.c, buf.get(), len + 1);

                memcpy(value.csl.parsed, parsed_buf.get(), sizeof(size_t) * new_parsed_len);
                if (phase_suffix_allowed)
                    memcpy(value.csl.phases, phases_buf.get(), sizeof(size_t) * new_parsed_len);
                value.csl.parsed_len = new_parsed_len;
                return requires_reboot ? ChangeConfigurationResponseStatus::REBOOT_REQUIRED : ChangeConfigurationResponseStatus::ACCEPTED;
            }
    }
    return ChangeConfigurationResponseStatus::REJECTED;
}

static const char * const connectorPhaseRotationStrings[] = {
    "NotApplicable",
    "Unknown",
    "RST",
    "RTS",
    "SRT",
    "STR",
    "TRS",
    "TSR",
};

// keep in sync with ocpp_plaform_gui.py config_key_strings
const char * const config_keys[OCPP_CONFIG_COUNT] {
    // CORE PROFILE
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
    "MessageTimeout",
    "MeterValuesAlignedData",
    "MeterValuesAlignedDataMaxLength",
    "MeterValuesSampledData",
    "MeterValuesSampledDataMaxLength",
    "MeterValueSampleInterval",
    //"MinimumStatusDuration",
    "NumberOfConnectors",
    "ResetRetries",
    "StopTransactionOnEVSideDisconnect",
    "StopTransactionOnInvalidId",
    "StopTransactionMaxMeterValues",
    "StopTxnAlignedData",
    "StopTxnAlignedDataMaxLength",
    "StopTxnSampledData",
    "StopTxnSampledDataMaxLength",
    "SupportedFeatureProfiles",
    //"SupportedFeatureProfilesMaxLength",
    "TransactionMessageAttempts",
    "TransactionMessageRetryInterval",
    "UnlockConnectorOnEVSideDisconnect",
    "WebSocketPingInterval",

    // // LOCAL AUTH LIST MANAGEMENT PROFILE
    //"LocalAuthListEnabled",
    //"LocalAuthListMaxLength",
    //"SendLocalListMaxLength",

    // // RESERVATION PROFILE
    //"ReserveConnectorZeroSupported",

    // SMART CHARGING PROFILE
    "ChargeProfileMaxStackLevel",
    "ChargingScheduleAllowedChargingRateUnit",
    "ChargingScheduleMaxPeriods",
    "ConnectorSwitch3to1PhaseSupported",
    "MaxChargingProfilesInstalled",
};

//TODO: implement that CSL max_elements and the corresponding ...MaxLength value are kept in sync

static OcppConfiguration config[OCPP_CONFIG_COUNT] = {
    // CORE PROFILE
    /*AllowOfflineTxForUnknownId*/        //OcppConfiguration::boolean(OCPP_DEFAULT_ALLOW_OFFLINE_TX_FOR_UNKNOWN_ID, false, false),
    /*AuthorizationCacheEnabled*/         //OcppConfiguration::boolean(OCPP_DEFAULT_AUTHORIZATION_CACHE_ENABLED, false, false),
    /*AuthorizeRemoteTxRequests*/         OcppConfiguration::boolean(OCPP_DEFAULT_AUTHORIZE_REMOTE_TX_REQUESTS, false, false),
    /*BlinkRepeat*/                       //OcppConfiguration::integer(OCPP_DEFAULT_BLINK_REPEAT, false, false),
    /*ClockAlignedDataInterval*/          OcppConfiguration::integer(OCPP_DEFAULT_CLOCK_ALIGNED_DATA_INTERVAL, false, false, 0),
    /*ConnectionTimeOut*/                 OcppConfiguration::integer(OCPP_DEFAULT_CONNECTION_TIME_OUT, false, false, 0, (UINT32_MAX / 2 - 1) / 1000),

                                          // +1 for index 0: "the phase rotation between
                                          //    the grid connection and the main energymeter"
                                          // +5 for two digits, the dot, comma and space
                                          // +1 for null-terminator
                                          // Format is "0.RST, 1.RST, 2.RTS"
    /*ConnectorPhaseRotation*/            OcppConfiguration::csl(OCPP_DEFAULT_CONNECTOR_PHASE_ROTATION, (OCPP_NUM_CONNECTORS + 1) * (strlen("NotApplicable") + 5) + 1, OCPP_NUM_CONNECTORS + 1, false, false, connectorPhaseRotationStrings, ARRAY_SIZE(connectorPhaseRotationStrings), true),
    /*ConnectorPhaseRotationMaxLength*/   OcppConfiguration::integer(OCPP_NUM_CONNECTORS + 1, true, false, 0),

    /*GetConfigurationMaxKeys*/           OcppConfiguration::integer(OCPP_CONFIG_COUNT, true, false, 0),
    /*HeartbeatInterval*/                 OcppConfiguration::integer(OCPP_DEFAULT_HEARTBEAT_INTERVAL_S, false, false, 0, (UINT32_MAX / 2 - 1) / 1000),
    /*LightIntensity*/                    //OcppConfiguration::integer(OCPP_DEFAULT_LIGHT_INTENSITY, false, false, 0),
    /*LocalAuthorizeOffline*/             OcppConfiguration::boolean(OCPP_DEFAULT_LOCAL_AUTHORIZE_OFFLINE, false, false),
    /*LocalPreAuthorize*/                 OcppConfiguration::boolean(OCPP_DEFAULT_LOCAL_PRE_AUTHORIZE, false, false),
    /*MaxEnergyOnInvalidId*/              //OcppConfiguration::integer(OCPP_DEFAULT_MAX_ENERGY_ON_INVALID_ID, false, false, 0),

    /*MessageTimeout*/                    OcppConfiguration::integer(OCPP_DEFAULT_MESSAGE_TIMEOUT, false, false, 1, (UINT32_MAX / 2 - 1) / 1000),

    // Its save to use the number of possible measurands as limit in elements,
    // because the complete list has a length of 465.
    // This also means that we don't have to implement the MeterValuesAlignedDataMaxLength key.
    /*MeterValuesAlignedData*/            OcppConfiguration::csl(OCPP_DEFAULT_METER_VALUES_ALIGNED_DATA, MAX_CONFIG_LENGTH, 5, false, false, SampledValueMeasurandStrings, (size_t)SampledValueMeasurand::NONE),
    /*MeterValuesAlignedDataMaxLength*/   OcppConfiguration::integer(OCPP_METER_VALUES_ALIGNED_DATA_MAX_LENGTH, true, false, 0),

    // Same reasoning as with MeterValuesAlignedData.
    /*MeterValuesSampledData*/            OcppConfiguration::csl(OCPP_DEFAULT_METER_VALUES_SAMPLED_DATA, MAX_CONFIG_LENGTH, 5, false, false, SampledValueMeasurandStrings, (size_t)SampledValueMeasurand::NONE, false, true),
    /*MeterValuesSampledDataMaxLength*/   OcppConfiguration::integer(OCPP_METER_VALUES_SAMPLED_DATA_MAX_LENGTH, true, false, 0),

    /*MeterValueSampleInterval*/          OcppConfiguration::integer(OCPP_DEFAULT_METER_VALUE_SAMPLE_INTERVAL, false, false, 0),
    /*MinimumStatusDuration*/             //OcppConfiguration::integer(OCPP_DEFAULT_MINIMUM_STATUS_DURATION, false, false, 0),
    /*NumberOfConnectors*/                OcppConfiguration::integer(OCPP_NUM_CONNECTORS, true, false, 0),
    /*ResetRetries*/                      OcppConfiguration::integer(OCPP_DEFAULT_RESET_RETRIES, false, false, 0),

    /*
    The description of the configuration key: StopTransactionOnEVSideDisconnect is required. It was added to
    OCPP to support EVs without lock at the car side. But this is now never the case, it was only the case with the
    first version of the Mitsubishi Outlander PHEV.
    The German eichrect does not allow this configuration key to be implemented.
    */
    /*StopTransactionOnEVSideDisconnect*/ OcppConfiguration::boolean(true, true, false),
    /*StopTransactionOnInvalidId*/        OcppConfiguration::boolean(OCPP_DEFAULT_STOP_TRANSACTION_ON_INVALID_ID, false, false),

    // Hardcode 2: This is the maximum amount of _meter values_ to send in a stoptxn.
    // However the size of the packet depends on which values to sample _per_ meter value.
    // To make sure we don't have to recalculate this if the configuration which values to sample changes,
    // we just abuse that the spec (errata 4.0) rules "The Start and Stop meter values SHALL never be dropped."
    // This should be sufficient in real-life, as we still periodically send meter values anyway.
    /*StopTransactionMaxMeterValues*/     OcppConfiguration::integer(OCPP_STOP_TRANSACTION_MAX_METER_VALUES, true, true),

    // Same reasoning as with MeterValuesAlignedData.
    /*StopTxnAlignedData*/                OcppConfiguration::csl("", MAX_CONFIG_LENGTH, 0, false, false, SampledValueMeasurandStrings, (size_t)SampledValueMeasurand::NONE),

    // Hardcode 0 for now: We don't want to support sending metering data in StopTransaction.req for now. The spec says:
    /*
    3.16.5. No metering data in a Stop Transaction
    When the configuration keys: StopTxnAlignedData and StopTxnSampledData are set to an empty string, the
    Charge Point SHALL not put meter values in a StopTransaction.req PDU.
    */
    // So we make sure that StopTxnAlignedData and StopTxnSampledData are always an empty string.
    /*StopTxnAlignedDataMaxLength*/       OcppConfiguration::integer(0, true, false, 0),

    // Same reasoning as with MeterValuesAlignedData.
    /*StopTxnSampledData*/                OcppConfiguration::csl("", MAX_CONFIG_LENGTH, 0, false, false, SampledValueMeasurandStrings, (size_t)SampledValueMeasurand::NONE),
    /*StopTxnSampledDataMaxLength*/       OcppConfiguration::integer(0, true, false, 0),

    /*SupportedFeatureProfiles*/          OcppConfiguration::csl(OCPP_SUPPORTED_FEATURE_PROFILES, strlen(OCPP_SUPPORTED_FEATURE_PROFILES) + 1, 2, true, false, nullptr, 0, false),
    /*SupportedFeatureProfilesMaxLength*/ //OcppConfiguration::integer(1, true), //errata 4.0: "This configuration key does not have to be implemented. It should not have been part of OCPP 1.6, "SupportedFeatureProfiles" is a readonly configuration key, false, 0."
    /*TransactionMessageAttempts*/        OcppConfiguration::integer(OCPP_DEFAULT_TRANSACTION_MESSAGE_ATTEMPTS, false, false, 0),
    /*TransactionMessageRetryInterval*/   OcppConfiguration::integer(OCPP_DEFAULT_TRANSACTION_MESSAGE_RETRY_INTERVAL, false, false, 0, (UINT32_MAX / 2 - 1) / 1000),
    /*UnlockConnectorOnEVSideDisconnect*/ OcppConfiguration::boolean(OCPP_DEFAULT_UNLOCK_CONNECTOR_ON_EV_SIDE_DISCONNECT, false, false),
    /*WebSocketPingInterval*/             OcppConfiguration::integer(OCPP_DEFAULT_WEB_SOCKET_PING_INTERVAL, false, false, 0, (UINT32_MAX / 2 - 1) / 1000),


    // // LOCAL AUTH LIST MANAGEMENT PROFILE
    /*LocalAuthListEnabled*/
    /*LocalAuthListMaxLength*/
    /*SendLocalListMaxLength*/

    // // RESERVATION PROFILE
    /*ReserveConnectorZeroSupported*/

    // SMART CHARGING PROFILE
    /*ChargeProfileMaxStackLevel*/        OcppConfiguration::integer(OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL, true, false, 0),
    /*ChargingScheduleAllowedChargingRateUnit*/ OcppConfiguration::csl(OCPP_CHARGING_SCHEDULE_ALLOWED_CHARGING_RATE_UNIT, strlen(OCPP_CHARGING_SCHEDULE_ALLOWED_CHARGING_RATE_UNIT) + 1, 1, true, false, nullptr, 0, false),
    /*ChargingScheduleMaxPeriods*/        OcppConfiguration::integer(OCPP_CHARGING_SCHEDULE_MAX_PERIODS, true, false, 0),
    /*ConnectorSwitch3to1PhaseSupported*/ OcppConfiguration::boolean(OCPP_CONNECTOR_SWITCH3TO1_PHASE_SUPPORTED, true, false),
    /*MaxChargingProfilesInstalled*/      OcppConfiguration::integer(OCPP_MAX_CHARGING_PROFILES_INSTALLED, true, false, 0),
};

OcppConfiguration& getConfig(ConfigKey key) {
    return config[(size_t)key];
}

OcppConfiguration& getConfig(size_t key) {
    return config[key];
}

uint32_t getIntConfigUnsigned(ConfigKey key) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::Integer) {
        log_error("Tried to read config %s (%d) as int, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::CSL ? "CSL" : "boolean");
        return 0xFFFFFFFF;
    }
    if (cfg.value.integer.min_ < 0)  {
        log_error("Tried to read config %s (%d) as unsigned int, but its allowed minimum is < 0", config_keys[(size_t) key], (int)key);
        return 0xFFFFFFFF;
    }
    return (uint32_t)cfg.value.integer.i;
}

int32_t getIntConfig(ConfigKey key) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::Integer) {
        log_error("Tried to read config %s (%d) as int, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::CSL ? "CSL" : "boolean");
        return -1;
    }
    return cfg.value.integer.i;
}

bool getBoolConfig(ConfigKey key) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::Boolean) {
        log_error("Tried to read config %s (%d) as bool, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::CSL ? "CSL" : "integer");
        return false;
    }
    return cfg.value.boolean.b;
}

size_t getCSLConfigLen(ConfigKey key) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::CSL) {
        log_error("Tried to read config %s (%d) as csl, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::Integer ? "integer" : "boolean");
        return 0;
    }
    return cfg.value.csl.parsed_len;
}

size_t *getCSLConfig(ConfigKey key) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::CSL) {
        log_error("Tried to read config %s (%d) as csl, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::Integer ? "integer" : "boolean");
        return nullptr;
    }
    return cfg.value.csl.parsed;
}

bool setIntConfig(ConfigKey key, int32_t i) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::Integer) {
        log_error("Tried to write config %s (%d) as int, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::CSL ? "CSL" : "boolean");
        return false;
    }

    cfg.value.integer.i = i;
    return true;
}

bool setBoolConfig(ConfigKey key, bool b) {
    OcppConfiguration &cfg = config[(size_t)key];
    if (cfg.type != OcppConfigurationValueType::Boolean) {
        log_error("Tried to write config %s (%d) as bool, but it is of type %s", config_keys[(size_t) key], (int)key, cfg.type == OcppConfigurationValueType::CSL ? "CSL" : "integer");
        return false;
    }
    cfg.value.boolean.b = b;
    return true;
}

void loadConfig()
{
    auto buf = heap_alloc_array<char>(8192);
    size_t len = platform_read_file("config", buf.get(), 8192);
    StaticJsonDocument<JSON_OBJECT_SIZE(MAX_SPECIFIED_CONFIGS)> doc;
    if (deserializeJson(doc, buf.get(), len) != DeserializationError::Ok)
        return;

    if (!doc.is<JsonObject>())
        return;

    JsonObject root = doc.as<JsonObject>();

    for (JsonPair kv : root) {
        size_t key_idx = 0;
        if (!lookup_key(&key_idx, kv.key().c_str(), config_keys, OCPP_CONFIG_COUNT)) {
            continue;
        }

        getConfig(key_idx).setValue(kv.value().as<const char *>());
    }
}

#ifdef OCPP_STATE_CALLBACKS
void debugDumpConfig() {
    size_t scratch_buf_size = OCPP_CONFIG_COUNT * 20;
    size_t scratch_buf_idx = 0;
    auto scratch_buf = heap_alloc_array<char>(scratch_buf_size);

    for(size_t i = 0; i < OCPP_CONFIG_COUNT; ++i) {
        auto &cfg = getConfig(i);
        switch(cfg.type) {
            case OcppConfigurationValueType::Boolean:
                platform_update_config_state((ConfigKey)i, cfg.value.boolean.b ? "true" : "false");
                break;
            case OcppConfigurationValueType::CSL:
                platform_update_config_state((ConfigKey)i, (const char *)cfg.value.csl.c);
                break;
            case OcppConfigurationValueType::Integer:
                char *val = scratch_buf.get() + scratch_buf_idx;

                int written = snprintf(val, scratch_buf_size - scratch_buf_idx, "%d", cfg.value.integer.i);
                if (written < 0) {
                    log_error("Failed to debug config %s: %d", config_keys[i], written);
                    break; //If this fails, we restore the default configuration of this key by not saving it.
                }

                platform_update_config_state((ConfigKey)i, (const char *)val);
                scratch_buf_idx += (size_t)written;
                ++scratch_buf_idx; // for null terminator
                break;

        }
    }
}
#endif

void saveConfig()
{
    StaticJsonDocument<JSON_OBJECT_SIZE(OCPP_CONFIG_COUNT)> doc;

    size_t scratch_buf_size = OCPP_CONFIG_COUNT * 20;
    size_t scratch_buf_idx = 0;
    auto scratch_buf = heap_alloc_array<char>(scratch_buf_size);

    for(size_t i = 0; i < OCPP_CONFIG_COUNT; ++i) {
        auto &cfg = getConfig(i);
        switch(cfg.type) {
            case OcppConfigurationValueType::Boolean:
                doc[config_keys[i]] = cfg.value.boolean.b ? "true" : "false";
                break;
            case OcppConfigurationValueType::CSL:
                doc[config_keys[i]] = (const char *)cfg.value.csl.c;
                break;
            case OcppConfigurationValueType::Integer:
                char *val = scratch_buf.get() + scratch_buf_idx;

                int written = snprintf(val, scratch_buf_size - scratch_buf_idx, "%d", cfg.value.integer.i);
                if (written < 0) {
                    log_error("Failed to save config %s: %d", config_keys[i], written);
                    break; //If this fails, we restore the default configuration of this key by not saving it.
                }

                doc[config_keys[i]] = (const char *)val;
                scratch_buf_idx += (size_t)written;
                ++scratch_buf_idx; // for null terminator
                break;

        }
    }

    auto buf_size = measureJson(doc);

    auto buf = heap_alloc_array<char>(buf_size);
    size_t written = serializeJson(doc, buf.get(), buf_size);
    platform_write_file("config", buf.get(), written);
}
