#pragma once

#include <limits>

#include "Connection.h"
#include "Configuration.h"
#include "Messages.h"
#include "Platform.h"
#include "Tools.h"
#include "Defines.h"
#include "Connector.h"
#include "ChargingProfile.h"

enum class OcppState {
    PowerOn, // send boot notification, wait for boot notification conf, don't do anything else
    FlushPersistentMessages, // boot notification received, accepted, but we have to send old txn messages first
    Idle, // boot notification received, accepted
    Pending, // boot notification received, pending
    Rejected, // boot notification received, rejected,
    Unavailable,
    Faulted,
    SoftReset,
    HardReset
};

class OcppChargePoint {
public:
    OcppChargePoint() {

    }

    OcppChargePoint (const OcppChargePoint&) = delete;

    void start(const char *websocket_endpoint_url, const char *charge_point_name_percent_encoded);

    void stop() {
        platform_disconnect(platform_ctx);
    }

    void handleTagSeen(int32_t connectorId, const char *tagId);
    void handleStop(int32_t connectorId, StopReason reason);

    void tick_power_on();
    void tick_flush_persistent_messages();
    void tick_idle();

    void tick_soft_reset();
    uint32_t soft_reset_start_ms = 0;

    void tick_hard_reset();
    uint32_t hard_reset_start_ms = 0;

    void tick_clock_aligned_meter_values();


    void tick();

    void onConnect();
    void onDisconnect();
    ChangeAvailabilityResponseStatus onChangeAvailability(ChangeAvailabilityType type);
    void saveAvailability();
    void loadAvailability();


    StatusNotificationStatus last_sent_status = StatusNotificationStatus::NONE;
    StatusNotificationStatus getStatus();
    void sendStatus();
    void forceSendStatus();

    bool sendCallAction(const ICall &call, time_t timestamp = 0, int32_t connectorId = 0);
    void onTimeout(CallAction action, uint32_t messageId, int32_t connectorId);

    // Core Profile
    CallResponse handleAuthorizeResponse(int32_t connectorId, AuthorizeResponseView conf);
    CallResponse handleBootNotificationResponse(int32_t connectorId, BootNotificationResponseView conf);
    CallResponse handleChangeAvailability(const char *uid, ChangeAvailabilityView req);
    CallResponse handleChangeConfiguration(const char *uid, ChangeConfigurationView req);
    CallResponse handleClearCache(const char *uid, ClearCacheView req);
    CallResponse handleDataTransfer(const char *uid, DataTransferView req);
    CallResponse handleDataTransferResponse(int32_t connectorId, DataTransferResponseView conf);
    CallResponse handleGetConfiguration(const char *uid, GetConfigurationView req);
    CallResponse handleHeartbeatResponse(int32_t connectorId, HeartbeatResponseView conf);
    CallResponse handleMeterValuesResponse(int32_t connectorId, MeterValuesResponseView conf);
    CallResponse handleRemoteStartTransaction(const char *uid, RemoteStartTransactionView req);
    CallResponse handleRemoteStopTransaction(const char *uid, RemoteStopTransactionView req);
    CallResponse handleReset(const char *uid, ResetView req);
    CallResponse handleStartTransactionResponse(int32_t connectorId, StartTransactionResponseView conf);
    CallResponse handleStatusNotificationResponse(int32_t connectorId, StatusNotificationResponseView conf);
    CallResponse handleStopTransactionResponse(int32_t connectorId, StopTransactionResponseView conf);
    CallResponse handleUnlockConnector(const char *uid, UnlockConnectorView req);

    // Smart Charging Profile
    CallResponse handleClearChargingProfile(const char *uid, ClearChargingProfileView req);
    CallResponse handleGetCompositeSchedule(const char *uid, GetCompositeScheduleView req);
    CallResponse handleSetChargingProfile(const char *uid, SetChargingProfileView req);

    struct EvalChargingProfilesResult {
        time_t nextCheck;
        float allocatedLimit[NUM_CONNECTORS + 1];
        int32_t allocatedPhases[NUM_CONNECTORS + 1];
    };

    EvalChargingProfilesResult evalChargingProfiles(time_t timeToEval);
    void evalAndApplyChargingProfiles();
    void triggerChargingProfileEval();

    OcppState state = OcppState::PowerOn;

    void *platform_ctx;

    uint32_t last_bn_send_ms = 0;

    OcppConnection connection;

    /*
    Connectors numbering (ConnectorIds) MUST be as follows:
    • ID of the first connector MUST be 1
    • Additional connectors MUST be sequentially numbered (no numbers may be skipped)
    • ConnectorIds MUST never be higher than the total number of connectors of a Charge Point
    • For operations intiated by the Central System, ConnectorId 0 is reserved for addressing the entire Charge
    Point.
    • For operations initiated by the Charge Point (when reporting), ConnectorId 0 is reserved for the Charge
    Point main controller.
    */
    Connector connectors[NUM_CONNECTORS];

    // +1 as stack levels 0 up to (including) CHARGE_PROFILE_MAX_STACK_LEVEL are allowed.
    Opt<ChargingProfile> chargePointMaxProfiles[CHARGE_PROFILE_MAX_STACK_LEVEL + 1];
    Opt<ChargingProfile> txDefaultProfiles[CHARGE_PROFILE_MAX_STACK_LEVEL + 1];

    time_t next_profile_eval = 0;
};
