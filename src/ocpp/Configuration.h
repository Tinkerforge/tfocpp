#pragma once

#include <stdint.h>
#include <stddef.h>

#include <memory>
#include <limits>

#include "Messages.h"

enum class OcppConfigurationValueType {
    Integer,
    Boolean,
    CSL
};

struct OcppConfiguration {
    OcppConfigurationValueType type;
    union {
        struct {
            int32_t i;
            int32_t min_;
            int32_t max_;
        } integer;
        struct {
            bool b;
        } boolean;
        struct {
            // csl as string ready to be sent with getconfiguration etc.
            char *c;
            size_t len;

            // parsed enum indices. buffer has to have size max_num_allowed_values
            size_t *parsed;
            size_t parsed_len;
            /* Where applicable, the Measurand is combined with the optional phase; for instance: Voltage.L1 */
            // For MeterValuesSampledData. Has length of parsed_len (one phase per parsed value, SampledValuePhase::NONE if no phase was passed)
            size_t *phases;

            // valid enum value strings
            const char * const *allowed_values;
            size_t allowed_values_len;

            // maximum number of allowed values. this is what gets reported as for example ConnectorPhaseRotationMaxLength
            size_t max_num_allowed_values;

            // is every value prefixed with the index? See ConnectorPhaseRotation
            bool prefix_index;
        } csl;
    } value;

    bool readonly;
    bool requires_reboot;

    static OcppConfiguration integer(int32_t value,
                                     bool readonly,
                                     bool requires_reboot,
                                     int32_t min_=std::numeric_limits<int32_t>::min(),
                                     int32_t max_=std::numeric_limits<int32_t>::max());

    static OcppConfiguration boolean(bool value,
                                     bool readonly,
                                     bool requires_reboot);

    static OcppConfiguration csl(const char *value,
                                 size_t max_len,
                                 size_t max_elements,
                                 bool readonly,
                                 bool requires_reboot,
                                 const char * const *allowed_values,
                                 size_t allowed_values_len,
                                 bool prefix_index = false,
                                 bool suffix_phase = false);

    ChangeConfigurationResponseStatus setValue(const char *newValue);
};

enum class ConnectorPhaseRotation {
    NotApplicable,
    Unknown,
    RST,
    RTS,
    SRT,
    STR,
    TRS,
    TSR,
};

enum class ConfigKey {
    // CORE PROFILE
    //AllowOfflineTxForUnknownId,
    //AuthorizationCacheEnabled,
    AuthorizeRemoteTxRequests,
    //BlinkRepeat,
    ClockAlignedDataInterval,
    ConnectionTimeOut,
    ConnectorPhaseRotation,
    ConnectorPhaseRotationMaxLength,
    GetConfigurationMaxKeys,
    HeartbeatInterval,
    //LightIntensity,
    LocalAuthorizeOffline,
    LocalPreAuthorize,
    //MaxEnergyOnInvalidId,
    MessageTimeout,
    MeterValuesAlignedData,
    MeterValuesAlignedDataMaxLength,
    MeterValuesSampledData,
    MeterValuesSampledDataMaxLength,
    MeterValueSampleInterval,
    //MinimumStatusDuration,
    NumberOfConnectors,
    ResetRetries,
    StopTransactionOnEVSideDisconnect,
    StopTransactionOnInvalidId,
    StopTransactionMeterMaxValues,
    StopTxnAlignedData,
    StopTxnAlignedDataMaxLength,
    StopTxnSampledData,
    StopTxnSampledDataMaxLength,
    SupportedFeatureProfiles,
    //SupportedFeatureProfilesMaxLength,
    TransactionMessageAttempts,
    TransactionMessageRetryInterval,
    UnlockConnectorOnEVSideDisconnect,
    WebSocketPingInterval,

    // // LOCAL AUTH LIST MANAGEMENT PROFILE
    // LocalAuthListEnabled,
    // LocalAuthListMaxLength,
    // SendLocalListMaxLength,

    // // RESERVATION PROFILE
    // ReserveConnectorZeroSupported,

    // SMART CHARGING PROFILE
    ChargeProfileMaxStackLevel,
    ChargingScheduleAllowedChargingRateUnit,
    ChargingScheduleMaxPeriods,
    ConnectorSwitch3to1PhaseSupported,
    MaxChargingProfilesInstalled,

    CONFIG_KEY_MAX
};

#define OCPP_CONFIG_COUNT ((size_t)ConfigKey::CONFIG_KEY_MAX)

extern const char * const config_keys[OCPP_CONFIG_COUNT];

OcppConfiguration& getConfig(size_t key);
OcppConfiguration& getConfig(ConfigKey key);
int32_t getIntConfig(ConfigKey key);
uint32_t getIntConfigUnsigned(ConfigKey key);
bool getBoolConfig(ConfigKey key);
size_t getCSLConfigLen(ConfigKey key);
size_t *getCSLConfig(ConfigKey key);
bool setIntConfig(ConfigKey key, int32_t i);
bool setBoolConfig(ConfigKey key, bool b);

void loadConfig();
void saveConfig();

#ifdef OCPP_STATE_CALLBACKS
void debugDumpConfig();
#endif
