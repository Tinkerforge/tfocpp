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
    WAITING_FOR_TAG,
    AUTHORIZING_FOR_START_C,
    AUTHORIZING_FOR_START_NC,
    WAITING_FOR_CABLE,
    STARTING,
    TRANSACTION, //This represents charging, suspended EVSE and suspended EV, as those are not controlled by us
    AUTHORIZING_FOR_STOP,
    STOPPING,
    STOPPING_NT,
    FINISHING
};

class Ocpp;

struct Connector {
    /*For ConnectorId 0, only a limited set is applicable, namely: Available, Unavailable and Faulted.
    The status of ConnectorId 0 has no direct connection to the status of the individual Connectors (>0).*/
    int32_t connectorId;

    ConnectorState state = ConnectorState::IDLE;
    StatusNotificationStatus last_sent_status = StatusNotificationStatus::NONE;

    IdTagInfo authorized_for;

    int32_t tag_deadline = 0;
    int32_t cable_deadline = 0;
    int32_t transaction_id = -1;

    uint32_t waiting_for_message_id = 0;

    Ocpp *ocpp = nullptr;

    void deauth();

    void setState(ConnectorState newState);
    void sendStatus(StatusNotificationStatus newStatus, StatusNotificationErrorCode error = StatusNotificationErrorCode::NO_ERROR, const char info[51] = nullptr);
    void sendCallAction(CallAction action, const DynamicJsonDocument &doc);

    void onTagSeen(const char *tag_id);

    void onAuthorizeConf(IdTagInfo info);

    void onStartTransactionConf(IdTagInfo info, int32_t transaction_id);

    StatusNotificationStatus getStatus();

    void tick();
};
