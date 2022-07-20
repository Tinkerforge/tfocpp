#pragma once

#include <stdint.h>

#include "OcppConfiguration.h"
#include "OcppPlatform.h"
#include "OcppMessages.h"
#include "OcppConnection.h"


#if 0
#define VALID_STATUS_STRIDE 9
bool valid_status_transitions[VALID_STATUS_STRIDE * VALID_STATUS_STRIDE] = {
/*From           To Avail  Prep   Charge SuspEV SuEVSE Finish Reserv Unavai Fault */
/*Available    */   false, true , true , true , true , false, true , true , true ,
/*Preparing    */   true , false, true , true , true , true , false, false, false,
/*Charging     */   true , false, false, true , true , true , false, true , true ,
/*SuspendedEV  */   true , false, true , false, true , true , false, true , true ,
/*SuspendedEVSE*/   true , false, true , true , false, true , false, true , true ,
/*Finishing    */   true , true , false, false, false, false, false, true , true ,
/*Reserved     */   true , true , false, false, false, false, false, true , true ,
/*Unavailable  */   true , true , true , true , true , false, false, false, true ,
/*Faulted      */   true , true , true , true , true , true , true , true , false,
};
#endif

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
    bool transaction_with_invalid_tag_id = false;
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

    void onTagSeen(const char *tag_id);
    void onStop(StopReason reason);

    void onAuthorizeConf(IdTagInfo info);

    void onStartTransactionConf(IdTagInfo info, int32_t txn_id);

    StatusNotificationStatus getStatus();

    void tick();
};
