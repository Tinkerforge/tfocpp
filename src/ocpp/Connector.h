#pragma once

#include <stdint.h>

#include "Configuration.h"
#include "Messages.h"
#include "Connection.h"
#include "MeterValueHandler.h"
#include "ChargingProfile.h"
#include "Defines.h"

enum class ConnectorState {
    IDLE,

    NO_CABLE_NO_TAG,
    NO_TAG,
    AUTH_START_NO_PLUG,
    AUTH_START_NO_CABLE,
    AUTH_START,
    NO_PLUG,
    NO_CABLE,

    TRANSACTION,  //This represents charging, suspended EVSE and suspended EV, as those are not controlled by us
    AUTH_STOP,

    FINISHING_UNLOCKED,
    FINISHING_NO_CABLE_UNLOCKED,
    FINISHING_NO_CABLE_LOCKED,
    FINISHING_NO_SAME_TAG,

    UNAVAILABLE
};

class OcppChargePoint;

struct Connector {
    void init(int32_t connId, OcppChargePoint *chargePoint);

    /*
    For ConnectorId 0, only a limited set is applicable, namely: Available, Unavailable and Faulted.
    The status of ConnectorId 0 has no direct connection to the status of the individual Connectors (>0).
    */
    int32_t connectorId;

    OcppChargePoint *cp = nullptr;

    OcppMeterValueHandler meter_value_handler;

    ConnectorState state = ConnectorState::IDLE;
    StatusNotificationStatus last_sent_status = StatusNotificationStatus::NONE;

    char tagIdInFlight[21] = {0};
    IdTagInfo authorized_for;

    uint32_t tag_deadline = 0;
    uint32_t cable_deadline = 0;
    int32_t transaction_id = INT32_MAX;
    uint64_t transaction_confirmed_id = 0;
    time_t transaction_start_time = 0;

    uint32_t current_allowed = OCPP_MAX_CHARGING_CURRENT;

    // +1 as stack levels 0 up to (including) OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL are allowed.
    Option<ChargingProfile> txProfiles[OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL + 1];
    Option<ChargingProfile> txDefaultProfiles[OCPP_CHARGE_PROFILE_MAX_STACK_LEVEL + 1];

    // This is set to true if a StartTransaction.conf is received that was not accepted
    // and StopTransactionOnInvalidId is not configured to true.
    // In this case we have to continue the transaction but supply no energy.
    // In the future we can use the (optional) config key MaxEnergyOnInvalidId here.
    bool transaction_with_non_accepted_tag_id = false;

    // This is set to true if we receive a ChangeAvailability.req that requests setting
    // us to unavailable, but the connector is currently locked.
    //
    // Note that this is _not_ exactly the same condition as the OCPP spec's
    /* When a transaction is in progress Charge Point SHALL respond with availability status 'Scheduled' to
       indicate that it is scheduled to occur after the transaction has finished. */
    // Additionally to the transaction states (TRANSACTION and AUTH_STOP) we also
    // have to consider here that we are finishing a transaction but the connector
    // is still locked (FINISHING_NO_SAME_TAG and FINISHING_NO_CABLE_LOCKED),
    // because the user has not yet proven that he owns the plugged in cable.
    //
    // If this flag is set and we go to any state that does not lock the connector,
    // go to UNAVAILABLE instead.
    //
    // If we receive a ChangeAvailability.req while in
    // FINISHING_NO_SAME_TAG or FINISHING_NO_CABLE_LOCKED
    // we lie to the central that the request is accepted
    // (not scheduled as it would be in the transaction states)
    // because unlocking the cable can only be done with the same tag
    // so without sending an Authorize.req.
    bool unavailable_requested = false;

    StopTransactionReason next_stop_reason = StopTransactionReason::NONE;

    void deauth();
    void setTagDeadline();
    void clearTagDeadline();
    void setCableDeadline();
    void clearCableDeadline();

    void setState(ConnectorState newState);
    void applyState();
    void sendStatus();
    void forceSendStatus();
    void sendCallAction(const ICall &call);

    bool isSelectableForRemoteStartTxn();
    bool canHandleRemoteStartTxn();
    bool canHandleRemoteStopTxn(int32_t transaction_id);

    void onTagSeen(const char *tag_id);
    void onStop(StopReason reason);

    void onAuthorizeError();
    void onAuthorizeConf(IdTagInfo info);

    void onStartTransactionConf(IdTagInfo info, int32_t txn_id);

    bool isTransactionActive();

    void onRemoteStartTransaction(const char *tag_id);
    void onRemoteStopTransaction();

    UnlockConnectorResponseStatus onUnlockConnector();

    ChangeAvailabilityResponseStatus onChangeAvailability(ChangeAvailabilityType type);

    bool willBeUnavailable() {
        return unavailable_requested || state == ConnectorState::UNAVAILABLE;
    }

    StatusNotificationStatus getStatus();

    void tick();

    void tickClockAlignedMeterValues();

    void tickSendClockAlignedMeterValues();
};
