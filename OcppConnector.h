#pragma once

#include <stdint.h>

#include "OcppConfiguration.h"
#include "OcppPlatform.h"
#include "OcppMessages.h"
#include "OcppConnection.h"

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
    /*For ConnectorId 0, only a limited set is applicable, namely: Available, Unavailable and Faulted.
    The status of ConnectorId 0 has no direct connection to the status of the individual Connectors (>0).*/
    int32_t connectorId;

    ConnectorState state = ConnectorState::IDLE;
    StatusNotificationStatus last_sent_status = StatusNotificationStatus::NONE;

    IdTagInfo authorized_for;

    uint32_t tag_deadline = 0;
    uint32_t cable_deadline = 0;
    int32_t transaction_id = -1;

    // This is set to true if a StartTransaction.conf is received that was not accepted
    // and StopTransactionOnInvalidId is not configured to true.
    // In this case we have to continue the transaction but supply no energy.
    // In the future we can use the (optional) config key MaxEnergyOnInvalidId here.
    bool transaction_with_invalid_tag_id = false;

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


    uint32_t waiting_for_message_id = 0;

    OcppChargePoint *cp = nullptr;

    void deauth();
    void setTagDeadline();
    void clearTagDeadline();
    void setCableDeadline();
    void clearCableDeadline();

    void setState(ConnectorState newState);
    void applyState();
    void sendStatus();
    void forceSendStatus();
    void sendCallAction(CallAction action, const DynamicJsonDocument &doc);

    bool isSelectableForRemoteStartTxn();
    bool canHandleRemoteStopTxn(int32_t transaction_id);

    void onTagSeen(const char *tag_id);
    void onStop(StopReason reason);

    void onAuthorizeConf(IdTagInfo info);

    void onStartTransactionConf(IdTagInfo info, int32_t txn_id);

    void onRemoteStartTransaction(const char *tag_id);
    void onRemoteStopTransaction();

    UnlockConnectorResponseStatus onUnlockConnector();

    ChangeAvailabilityResponseStatus onChangeAvailability(ChangeAvailabilityType type);

    StatusNotificationStatus getStatus();

    void tick();
};
