#include "OcppConnector.h"

#include "OcppTools.h"
#include "Ocpp.h"

void Connector::deauth() {
    authorized_for = IdTagInfo{};
}

const char * ConnectorState_Strings[] = {
    "IDLE",
    "WAITING_FOR_TAG",
    "AUTHORIZING_FOR_START_C",
    "AUTHORIZING_FOR_START_NC",
    "WAITING_FOR_CABLE",
    "STARTING",
    "TRANSACTION",
    "AUTHORIZING_FOR_STOP",
    "STOPPING",
    "STOPPING_NT",
    "FINISHING",
};

void Connector::setState(ConnectorState newState) {
    switch (newState) {
        case ConnectorState::IDLE:
            platform_unlock_cable(connectorId);
            deauth();
            tag_deadline = 0;
            cable_deadline = 0;
            transaction_id = -1;
            break;
        case ConnectorState::WAITING_FOR_TAG:
            // The spec does not specify explicitly which timeout to use here:
            // The configuration spec only talks about inserting the cable, however
            // transition B9 says "time out (configured by the configuration key: ConnectionTimeOut) on expected user action"
            tag_deadline = platform_now_ms() + getIntConfig(ConfigKey::ConnectionTimeOut) * 1000;
            break;
        case ConnectorState::AUTHORIZING_FOR_START_C:
            //tag_deadline = 0;
            cable_deadline = 0;
            break;
        case ConnectorState::AUTHORIZING_FOR_START_NC:
            // We have to set the timeout here: The spec says
            // "Interval *from beginning of status: 'Preparing' until incipient Transaction is automatically canceled, due to failure of EV driver to
            // (correctly) insert the charging cable connector(s) into the appropriate socket(s)."
            cable_deadline = platform_now_ms() + getIntConfig(ConfigKey::ConnectionTimeOut) * 1000;
            break;
        case ConnectorState::WAITING_FOR_CABLE:
            break;
        case ConnectorState::STARTING:
            tag_deadline = 0;
            cable_deadline = 0;

            // StartTransaction is the notification that we have started to charge, not the request for permission.
            platform_lock_cable(connectorId);
            platform_set_charging_current(connectorId, OCPP_PLATFORM_MAX_CHARGING_CURRENT);
            this->sendCallAction(CallAction::START_TRANSACTION, StartTransaction(connectorId, authorized_for.tagId, platform_get_energy(connectorId), platform_get_system_time(ocpp->platform_ctx)));
            break;
        case ConnectorState::TRANSACTION:
            break;
        case ConnectorState::AUTHORIZING_FOR_STOP:
            break;
        case ConnectorState::STOPPING:
            // TODO: this is guestimated without reading the spec. Figure out stoptransactionreason etc.
            this->sendCallAction(CallAction::STOP_TRANSACTION, StopTransaction(platform_get_energy(connectorId), platform_get_system_time(ocpp->platform_ctx), transaction_id, authorized_for.tagId));
            deauth();
            transaction_id = -1;
            break;
        case ConnectorState::STOPPING_NT:
            this->sendCallAction(CallAction::STOP_TRANSACTION, StopTransaction(platform_get_energy(connectorId), platform_get_system_time(ocpp->platform_ctx), transaction_id, authorized_for.tagId, StopTransactionReason::DE_AUTHORIZED));
            deauth();
            transaction_id = -1;
            break;
        case ConnectorState::FINISHING:
            platform_unlock_cable(connectorId);
            break;
    }
    platform_printfln("%s -> %s", ConnectorState_Strings[(int)state], ConnectorState_Strings[(int)newState]);

    state = newState;

    this->sendStatus(getStatus());
}

void Connector::sendStatus(StatusNotificationStatus newStatus, StatusNotificationErrorCode error, const char info[51]) {
    if (last_sent_status == newStatus)
        return;

    this->sendCallAction(CallAction::STATUS_NOTIFICATION, StatusNotification(connectorId, StatusNotificationErrorCode::NO_ERROR, newStatus, info, platform_get_system_time(ocpp->platform_ctx)));
    last_sent_status = newStatus;
}

void Connector::sendCallAction(CallAction action, const DynamicJsonDocument &doc)
{
    long id = std::atol(doc[1]);
    this->waiting_for_message_id = id;
    ocpp->sendCallAction(action, doc);
}

void Connector::onTagSeen(const char *tag_id) {
    switch (state) {
        case ConnectorState::WAITING_FOR_TAG:
            setState(ConnectorState::AUTHORIZING_FOR_START_C);
            authorized_for.updateTagId(tag_id);
            this->sendCallAction(CallAction::AUTHORIZE, Authorize(tag_id));
            break;
        case ConnectorState::IDLE:
            setState(ConnectorState::AUTHORIZING_FOR_START_NC);
            authorized_for.updateTagId(tag_id);
            this->sendCallAction(CallAction::AUTHORIZE, Authorize(tag_id));
            break;

        case ConnectorState::TRANSACTION:
            /*
            When stopping a Transaction, the
            Charge Point SHALL only send an Authorize.req when the identifier used for stopping the transaction is different
            from the identifier that started the transaction.
            */
            if (authorized_for.is_same_tag(tag_id)) {
                setState(ConnectorState::STOPPING);
                break;
            }

            setState(ConnectorState::AUTHORIZING_FOR_STOP);
            authorized_for.updateTagId(tag_id);
            this->sendCallAction(CallAction::AUTHORIZE, Authorize(tag_id));
            break;
        case ConnectorState::STOPPING_NT:
            if (authorized_for.is_same_tag(tag_id)) {
                setState(ConnectorState::FINISHING);
                break;
            }

            // This is interesting: If we started a transaction (offline),
            // the starttransaction.conf is received later and does not accept the tag,
            // only _the same tag_ (or one with the same parentId) may unlock the cable.
            break;
        case ConnectorState::WAITING_FOR_CABLE:
            if (authorized_for.is_same_tag(tag_id)) {
                setState(ConnectorState::IDLE);
                break;
            }

            // Also only accept the same tag here: This is aborting a transaction before
            // it has even started.
            break;

        default:
            break;
    }
}

void Connector::onAuthorizeConf(IdTagInfo info) {
    authorized_for.updateFromIdTagInfo(info);
    switch (state) {
        case ConnectorState::AUTHORIZING_FOR_START_C:
            if (info.status == ResponseIdTagInfoEntriesStatus::ACCEPTED) {
                setState(ConnectorState::STARTING);
                return;
            }

            setState(ConnectorState::WAITING_FOR_TAG);
            return;
        case ConnectorState::AUTHORIZING_FOR_START_NC:
            if (info.status == ResponseIdTagInfoEntriesStatus::ACCEPTED) {
                setState(ConnectorState::WAITING_FOR_CABLE);
                return;
            }

            setState(ConnectorState::IDLE);
            return;
        case ConnectorState::AUTHORIZING_FOR_STOP:
            if (info.status == ResponseIdTagInfoEntriesStatus::ACCEPTED) {
                setState(ConnectorState::STOPPING);
                return;
            }

            setState(ConnectorState::TRANSACTION);
            return;
        default:
            return;
    }
}

void Connector::onStartTransactionConf(IdTagInfo info, int32_t transaction_id) {
    if (state != ConnectorState::STARTING && state != ConnectorState::TRANSACTION)
        return;

    authorized_for.updateFromIdTagInfo(info);

    if (info.status == ResponseIdTagInfoEntriesStatus::ACCEPTED) {
        this->transaction_id = transaction_id;
        setState(ConnectorState::TRANSACTION);
        return;
    }

    if (getBoolConfig(ConfigKey::StopTransactionOnInvalidId)) {
        setState(ConnectorState::STOPPING_NT);
        return;
    }

    if (info.status == ResponseIdTagInfoEntriesStatus::INVALID) {
        // TODO: implement MaxEnergyOnInvalidId here
        platform_set_charging_current(connectorId, 0);
    }
    return;
}



StatusNotificationStatus Connector::getStatus() {
    EVSEState evse_state = platform_get_evse_state(connectorId);
    if (evse_state == EVSEState::Faulted)
        return StatusNotificationStatus::FAULTED;

    switch (state) {
        case ConnectorState::IDLE:
            return StatusNotificationStatus::AVAILABLE;

        case ConnectorState::WAITING_FOR_TAG:
        case ConnectorState::AUTHORIZING_FOR_START_C:
        case ConnectorState::AUTHORIZING_FOR_START_NC:
        case ConnectorState::WAITING_FOR_CABLE:
            return StatusNotificationStatus::PREPARING;

        case ConnectorState::STARTING:
        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTHORIZING_FOR_STOP:
            switch (evse_state) {
                case EVSEState::Connected:
                    return StatusNotificationStatus::SUSPENDED_EVSE;
                case EVSEState::ReadyToCharge:
                    return StatusNotificationStatus::SUSPENDED_EV;
                case EVSEState::Charging:
                    return StatusNotificationStatus::CHARGING;
                case EVSEState::NotConnected: // connector state will transition to idle in the next tick
                    return StatusNotificationStatus::AVAILABLE;
                case EVSEState::Faulted:
                    return StatusNotificationStatus::FAULTED;
            }

        case ConnectorState::STOPPING:
        case ConnectorState::STOPPING_NT:
        case ConnectorState::FINISHING:
            return StatusNotificationStatus::FINISHING;
    }
    return StatusNotificationStatus::NONE;
}

void Connector::tick() {
    EVSEState evse_state = platform_get_evse_state(connectorId);

    switch (state) {
        case ConnectorState::IDLE:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::WAITING_FOR_TAG);
                    break;
                default:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;
        case ConnectorState::WAITING_FOR_TAG:
        case ConnectorState::AUTHORIZING_FOR_START_C:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    setState(ConnectorState::IDLE);
                    break;
                case EVSEState::Connected:
                    break;
                default:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::AUTHORIZING_FOR_START_NC:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::AUTHORIZING_FOR_START_C);
                    break;
                default:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;
        case ConnectorState::WAITING_FOR_CABLE:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::STARTING);
                    break;
                default:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::STARTING:
        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTHORIZING_FOR_STOP:
        case ConnectorState::STOPPING:
        case ConnectorState::STOPPING_NT:
        case ConnectorState::FINISHING:
            if (evse_state == EVSEState::NotConnected)
                setState(ConnectorState::IDLE);
            break;
    }

    if (tag_deadline != 0 && deadline_elapsed(tag_deadline)) {
        setState(ConnectorState::FINISHING);
        platform_tag_timed_out(connectorId);
    }

    if (cable_deadline != 0 && deadline_elapsed(cable_deadline)) {
        setState(ConnectorState::IDLE);
        platform_cable_timed_out(connectorId);
    }

    this->sendStatus(getStatus());
}
