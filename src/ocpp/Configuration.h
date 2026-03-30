#pragma once

#include <stdint.h>
#include <stddef.h>

#include <memory>
#include <limits>

#include "Messages.h"
#include "Defines.h"

enum class OcppConfigurationValueType : uint8_t {
    Integer,
    Boolean,
    CSL,
    String
};

struct OcppConfiguration {
    OcppConfigurationValueType type;
    bool readonly;
    bool requires_reboot;
    bool hidden;

    ~OcppConfiguration() {
        if (this->type == OcppConfigurationValueType::CSL) {
            free(value.csl.c); value.csl.c = nullptr;
            free(value.csl.parsed); value.csl.parsed = nullptr;
            if (value.csl.phases != nullptr) {
                free(value.csl.phases); value.csl.phases = nullptr;
            }
        } else if (this->type == OcppConfigurationValueType::String) {
            free(value.string.s); value.string.s = nullptr;
        }
    }

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
            // Can't use std::unique_ptr here as this requires a non-trivial constructor.
            // c, parsed and phases are owned by csl.

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
        struct {
            // string ready to be sent with getconfiguration etc.
            char *s;
            size_t len;
            size_t max_len;
        } string;
    } value;

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

    static OcppConfiguration string(const char *value,
                                    size_t max_len,
                                    bool readonly,
                                    bool requires_reboot);

    ChangeConfigurationResponseStatus setValue(const char *newValue, bool force = false);
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

enum class PublicKeyWithSignedMeterValue : uint8_t {
    Never,
    OncePerTransaction,
    EveryMeterValue
};

static_assert(OCPP_NUM_CONNECTORS == 1, "only one connector is supported for now: each connector requires a separate MeterPublicKey[ConnectorID] ConfigKey");

enum class ConfigKey {
    // CORE PROFILE
    //AllowOfflineTxForUnknownId, // 9.1.1
    //AuthorizationCacheEnabled, // 9.1.2
    AuthorizeRemoteTxRequests, // 9.1.3
    //BlinkRepeat, // 9.1.4
    ClockAlignedDataInterval, // 9.1.5
    ConnectionTimeOut, // 9.1.6
    ConnectorPhaseRotation, // 9.1.7
    ConnectorPhaseRotationMaxLength, // 9.1.8
    GetConfigurationMaxKeys, // 9.1.9
    HeartbeatInterval, // 9.1.10
    //LightIntensity, // 9.1.11
    LocalAuthorizeOffline, // 9.1.12
    LocalPreAuthorize, // 9.1.13
    //MaxEnergyOnInvalidId, // 9.1.14
    MessageTimeout, // errata sheet v4.0 3.83 (there defined as 9.1.15)
    MeterValuesAlignedData, // 9.1.15
    MeterValuesAlignedDataMaxLength, // 9.1.16
    MeterValuesSampledData, // 9.1.17
    MeterValuesSampledDataMaxLength, // 9.1.18
    MeterValueSampleInterval, // 9.1.19
    //MinimumStatusDuration, // 9.1.20
    NumberOfConnectors, // 9.1.21
    ResetRetries, // 9.1.22
    StopTransactionMaxMeterValues, // errata sheet v4.0 3.91 (there defined as 9.1.23)
    StopTransactionOnEVSideDisconnect, // 9.1.23
    StopTransactionOnInvalidId, // 9.1.24
    StopTxnAlignedData, // 9.1.25
    StopTxnAlignedDataMaxLength, // 9.1.26
    StopTxnSampledData, // 9.1.27
    StopTxnSampledDataMaxLength, // 9.1.28
    SupportedFeatureProfiles, // 9.1.29
    //SupportedFeatureProfilesMaxLength, // 9.1.30 (removed in errata sheet v4.0)
    TransactionMessageAttempts, // 9.1.31
    TransactionMessageRetryInterval, // 9.1.32
    UnlockConnectorOnEVSideDisconnect, // 9.1.33
    WebSocketPingInterval, // 9.1.34

    // // LOCAL AUTH LIST MANAGEMENT PROFILE
    // LocalAuthListEnabled, // 9.2.1
    // LocalAuthListMaxLength, // 9.2.2
    // SendLocalListMaxLength, // 9.2.3

    // // RESERVATION PROFILE
    // ReserveConnectorZeroSupported, // 9.3.1

    // SMART CHARGING PROFILE
    ChargeProfileMaxStackLevel, // 9.4.1
    ChargingScheduleAllowedChargingRateUnit, // 9.4.2
    ChargingScheduleMaxPeriods, // 9.4.3
    ConnectorSwitch3to1PhaseSupported, // 9.4.4
    MaxChargingProfilesInstalled, // 9.4.5

    // FIRMWARE MANAGEMENT PROFILE
    //SupportedFileTransferProtocols, // 9.5.1 (errata sheet v4.0 3.90)

    // SIGNED METER VALUES
    MeterPublicKey1, // Signed Meter Values in OCPP 3.3.1
    PublicKeyWithSignedMeterValue, // Signed Meter Values in OCPP 3.3.2
    SampledDataSignReadings, // Signed Meter Values in OCPP 3.3.3
    StartTxnSampledData, // Signed Meter Values in OCPP 3.3.4
    SampledDataSignStartedReadings, // Signed Meter Values in OCPP 3.3.5
    SampledDataSignUpdatedReadings, // Signed Meter Values in OCPP 3.3.6
    AlignedDataSignReadings, // Signed Meter Values in OCPP 3.3.7
    AlignedDataSignUpdatedReadings, // Signed Meter Values in OCPP 3.3.8

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
size_t *getCSLPhases(ConfigKey key);
const char *getStringConfig(ConfigKey key);
bool setIntConfig(ConfigKey key, int32_t i);
bool setBoolConfig(ConfigKey key, bool b);

std::unique_ptr<OcppConfiguration[]> loadConfig();
void saveConfig();

#ifdef OCPP_STATE_CALLBACKS
void debugDumpConfig();
#endif
