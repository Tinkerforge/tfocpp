#include "OcppConnector.h"

#include "OcppTools.h"
#include "OcppChargePoint.h"

void Connector::deauth() {
    authorized_for = IdTagInfo{};
}

void Connector::setTagDeadline()
{
    // The spec does not specify explicitly which timeout to use here:
    // The configuration spec only talks about inserting the cable, however
    // transition B9 says "time out (configured by the configuration key: ConnectionTimeOut) on expected user action"
    if (tag_deadline == 0)
        tag_deadline = platform_now_ms() + getIntConfig(ConfigKey::ConnectionTimeOut) * 1000;
}

void Connector::clearTagDeadline()
{
    tag_deadline = 0;
}

void Connector::setCableDeadline()
{
    if (cable_deadline == 0)
        cable_deadline = platform_now_ms() + getIntConfig(ConfigKey::ConnectionTimeOut) * 1000;
}

void Connector::clearCableDeadline()
{
    cable_deadline = 0;
}

static const char * ConnectorState_Strings[] = {
    "IDLE",
    "NO_CABLE_NO_TAG",
    "NO_TAG",
    "AUTH_START_NO_PLUG",
    "AUTH_START_NO_CABLE",
    "AUTH_START",
    "NO_PLUG",
    "NO_CABLE",
    "TRANSACTION",
    "AUTH_STOP",
    "FINISHING_UNLOCKED",
    "FINISHING_NO_CABLE_UNLOCKED",
    "FINISHING_NO_CABLE_LOCKED",
    "FINISHING_NO_SAME_TAG"
};

void Connector::applyState() {
    switch (state) {
        case ConnectorState::IDLE:
        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
            platform_unlock_cable(connectorId);
            platform_set_charging_current(connectorId, 0);

            clearTagDeadline();
            clearCableDeadline();

            deauth();

            transaction_id = -1;
            transaction_with_invalid_tag_id = false;
            break;

        case ConnectorState::NO_CABLE_NO_TAG:
            platform_unlock_cable(connectorId);
            platform_set_charging_current(connectorId, 0);

            setTagDeadline();
            setCableDeadline();

            deauth();

            transaction_id = -1;
            transaction_with_invalid_tag_id = false;
            break;

        case ConnectorState::NO_TAG:
            platform_unlock_cable(connectorId);
            platform_set_charging_current(connectorId, 0);

            setTagDeadline();
            clearCableDeadline();

            deauth();

            transaction_id = -1;
            transaction_with_invalid_tag_id = false;
            break;

        case ConnectorState::AUTH_START_NO_PLUG:
        case ConnectorState::AUTH_START_NO_CABLE:
            platform_unlock_cable(connectorId);
            platform_set_charging_current(connectorId, 0);

            // We have to set the timeout here: The spec says
            // "Interval *from beginning of status: 'Preparing' until incipient Transaction is automatically canceled, due to failure of EV driver to
            // (correctly) insert the charging cable connector(s) into the appropriate socket(s)."
            setTagDeadline();
            setCableDeadline();

            transaction_id = -1;
            transaction_with_invalid_tag_id = false;
            break;

        case ConnectorState::AUTH_START:
            platform_unlock_cable(connectorId);
            platform_set_charging_current(connectorId, 0);

            setTagDeadline();
            clearCableDeadline();

            transaction_id = -1;
            transaction_with_invalid_tag_id = false;
            break;

        case ConnectorState::NO_PLUG:
        case ConnectorState::NO_CABLE:
            platform_unlock_cable(connectorId);
            platform_set_charging_current(connectorId, 0);

            clearTagDeadline();
            setCableDeadline();

            transaction_id = -1;
            transaction_with_invalid_tag_id = false;
            break;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            platform_lock_cable(connectorId);
            // TODO: implement MaxEnergyOnInvalidId here
            platform_set_charging_current(connectorId, transaction_with_invalid_tag_id ? 0 : OCPP_PLATFORM_MAX_CHARGING_CURRENT);

            clearTagDeadline();
            clearCableDeadline();
            break;

        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            platform_lock_cable(connectorId);
            platform_set_charging_current(connectorId, 0);

            clearTagDeadline();
            clearCableDeadline();

            transaction_id = -1;
            transaction_with_invalid_tag_id = false;
            break;
    }
}

void Connector::setState(ConnectorState newState) {
    platform_printfln("%s -> %s", ConnectorState_Strings[(int)state], ConnectorState_Strings[(int)newState]);
    ConnectorState oldState = state;
    state = newState;

    switch (newState) {
        case ConnectorState::AUTH_START_NO_PLUG:
        case ConnectorState::AUTH_START_NO_CABLE:
        case ConnectorState::AUTH_START:
            switch (oldState) {
                case ConnectorState::IDLE:
                case ConnectorState::NO_CABLE_NO_TAG:
                case ConnectorState::NO_TAG:
                case ConnectorState::FINISHING_UNLOCKED:
                case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
                    this->sendCallAction(CallAction::AUTHORIZE, Authorize(authorized_for.tagId));
                    break;
                case ConnectorState::TRANSACTION:
                case ConnectorState::AUTH_STOP:
                case ConnectorState::AUTH_START_NO_PLUG:
                case ConnectorState::AUTH_START_NO_CABLE:
                case ConnectorState::AUTH_START:
                case ConnectorState::NO_PLUG:
                case ConnectorState::NO_CABLE:
                case ConnectorState::FINISHING_NO_CABLE_LOCKED:
                case ConnectorState::FINISHING_NO_SAME_TAG:
                    break;
            }
            break;
        case ConnectorState::TRANSACTION:
            switch (oldState) {
                case ConnectorState::AUTH_START:
                case ConnectorState::NO_PLUG:
                case ConnectorState::NO_CABLE:
                    this->sendCallAction(CallAction::START_TRANSACTION, StartTransaction(connectorId, authorized_for.tagId, platform_get_energy(connectorId), platform_get_system_time(cp->platform_ctx)));
                    break;

                case ConnectorState::IDLE:
                case ConnectorState::NO_CABLE_NO_TAG:
                case ConnectorState::NO_TAG:
                case ConnectorState::AUTH_START_NO_PLUG:
                case ConnectorState::AUTH_START_NO_CABLE:
                case ConnectorState::TRANSACTION:
                case ConnectorState::AUTH_STOP:
                case ConnectorState::FINISHING_UNLOCKED:
                case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
                case ConnectorState::FINISHING_NO_CABLE_LOCKED:
                case ConnectorState::FINISHING_NO_SAME_TAG:
                    break;
            }
            break;

        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
             switch (oldState) {
                case ConnectorState::TRANSACTION:
                case ConnectorState::AUTH_STOP:
                    if (this->next_stop_reason == StopTransactionReason::NONE) {
                        platform_printfln("Attempting to send stop transaction but next stop reason is none!");
                    }
                    this->sendCallAction(CallAction::STOP_TRANSACTION, StopTransaction(platform_get_energy(connectorId), platform_get_system_time(cp->platform_ctx), transaction_id, authorized_for.tagId, this->next_stop_reason));
                    break;

                case ConnectorState::IDLE:
                case ConnectorState::NO_CABLE_NO_TAG:
                case ConnectorState::NO_TAG:
                case ConnectorState::AUTH_START_NO_PLUG:
                case ConnectorState::AUTH_START_NO_CABLE:
                case ConnectorState::AUTH_START:
                case ConnectorState::NO_PLUG:
                case ConnectorState::NO_CABLE:
                case ConnectorState::FINISHING_UNLOCKED:
                case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
                case ConnectorState::FINISHING_NO_CABLE_LOCKED:
                case ConnectorState::FINISHING_NO_SAME_TAG:
                    break;
            }
            break;

        case ConnectorState::IDLE:
        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::NO_TAG:
        case ConnectorState::NO_PLUG:
        case ConnectorState::NO_CABLE:
        case ConnectorState::AUTH_STOP:
            break;
    }
}

void Connector::sendStatus() {
    StatusNotificationStatus newStatus = getStatus();
    if (last_sent_status == newStatus)
        return;

    forceSendStatus();
}

void Connector::forceSendStatus()
{
    StatusNotificationStatus newStatus = getStatus();
    platform_printfln("Sending status %s for connector %d", StatusNotificationStatusStrings[(size_t)newStatus], connectorId);

    this->sendCallAction(CallAction::STATUS_NOTIFICATION, StatusNotification(connectorId, StatusNotificationErrorCode::NO_ERROR, newStatus, nullptr, platform_get_system_time(cp->platform_ctx)));
    last_sent_status = newStatus;
}

void Connector::sendCallAction(CallAction action, const DynamicJsonDocument &doc)
{
    long id = std::atol(doc[1]);
    this->waiting_for_message_id = (uint32_t)id;
    cp->sendCallAction(action, doc);
}

StatusNotificationStatus Connector::getStatus() {
    EVSEState evse_state = platform_get_evse_state(connectorId);
    if (evse_state == EVSEState::Faulted)
        return StatusNotificationStatus::FAULTED;

    // TODO: implemented unavailable and reserved

    switch (state) {
        case ConnectorState::IDLE:
            return StatusNotificationStatus::AVAILABLE;

        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::NO_TAG:
        case ConnectorState::AUTH_START_NO_PLUG:
        case ConnectorState::AUTH_START_NO_CABLE:
        case ConnectorState::AUTH_START:
        case ConnectorState::NO_PLUG:
        case ConnectorState::NO_CABLE:
            return StatusNotificationStatus::PREPARING;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            switch (evse_state) {
                case EVSEState::Connected:
                    return StatusNotificationStatus::SUSPENDED_EVSE;
                case EVSEState::ReadyToCharge:
                    return StatusNotificationStatus::SUSPENDED_EV;
                case EVSEState::Charging:
                    return StatusNotificationStatus::CHARGING;
                case EVSEState::PlugDetected: // connector state will transition to finishing* in the next tick
                    return StatusNotificationStatus::FINISHING;
                case EVSEState::NotConnected: // connector state will transition to idle in the next tick
                    return StatusNotificationStatus::AVAILABLE;
                case EVSEState::Faulted:
                    return StatusNotificationStatus::FAULTED;
            }

        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            return StatusNotificationStatus::FINISHING;
    }
    return StatusNotificationStatus::NONE;
}

// Handles PlugIn, PlugOut, CableIn, CableOut, TagDeadlineElapsed, CableDeadlineElapsed
void Connector::tick() {
    EVSEState evse_state = platform_get_evse_state(connectorId);

    switch (state) {
        case ConnectorState::IDLE:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    break;
                case EVSEState::PlugDetected:
                    setState(ConnectorState::NO_CABLE_NO_TAG);
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::NO_TAG);
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;
        case ConnectorState::NO_CABLE_NO_TAG:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    setState(ConnectorState::IDLE);
                    break;
                case EVSEState::PlugDetected:
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::NO_TAG);
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::NO_TAG:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    setState(ConnectorState::IDLE);
                    break;
                case EVSEState::PlugDetected:
                    setState(ConnectorState::NO_CABLE_NO_TAG);
                    break;
                case EVSEState::Connected:
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::AUTH_START_NO_PLUG:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    break;
                case EVSEState::PlugDetected:
                    setState(ConnectorState::AUTH_START_NO_CABLE);
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::AUTH_START);
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::AUTH_START_NO_CABLE:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    setState(ConnectorState::AUTH_START_NO_PLUG);
                    break;
                case EVSEState::PlugDetected:
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::AUTH_START);
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::AUTH_START:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    setState(ConnectorState::AUTH_START_NO_PLUG);
                    break;
                case EVSEState::PlugDetected:
                    setState(ConnectorState::AUTH_START_NO_CABLE);
                    break;
                case EVSEState::Connected:
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::NO_PLUG:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    break;
                case EVSEState::PlugDetected:
                    setState(ConnectorState::AUTH_START_NO_CABLE);
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::TRANSACTION);
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::NO_CABLE:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    setState(ConnectorState::NO_PLUG);
                    break;
                case EVSEState::PlugDetected:
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::TRANSACTION);
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::TRANSACTION:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    platform_printfln("Aborting transaction!");
                    setState(ConnectorState::IDLE);
                    break;
                case EVSEState::PlugDetected:
                    this->next_stop_reason = StopTransactionReason::EV_DISCONNECTED;
                    setState(getBoolConfig(ConfigKey::UnlockConnectorOnEVSideDisconnect) ? ConnectorState::FINISHING_NO_CABLE_UNLOCKED : ConnectorState::FINISHING_NO_CABLE_LOCKED);
                    this->next_stop_reason = StopTransactionReason::NONE;
                    break;
                case EVSEState::Connected:
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                    break;
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::AUTH_STOP:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    platform_printfln("Aborting transaction!");
                    setState(ConnectorState::IDLE);
                    break;
                case EVSEState::PlugDetected:
                    this->next_stop_reason = StopTransactionReason::EV_DISCONNECTED;
                    setState(getBoolConfig(ConfigKey::UnlockConnectorOnEVSideDisconnect) ? ConnectorState::FINISHING_NO_CABLE_UNLOCKED : ConnectorState::FINISHING_NO_CABLE_LOCKED);
                    this->next_stop_reason = StopTransactionReason::NONE;
                    break;
                case EVSEState::Connected:
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                    break;
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::FINISHING_UNLOCKED:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    setState(ConnectorState::IDLE);
                    break;
                case EVSEState::PlugDetected:
                    setState(ConnectorState::FINISHING_NO_CABLE_UNLOCKED);
                    break;
                case EVSEState::Connected:
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    setState(ConnectorState::IDLE);
                    break;
                case EVSEState::PlugDetected:
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::NO_TAG);
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    setState(ConnectorState::IDLE);
                    break;
                case EVSEState::PlugDetected:
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::NO_TAG);
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

         case ConnectorState::FINISHING_NO_SAME_TAG:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    setState(ConnectorState::IDLE);
                    break;
                case EVSEState::PlugDetected:
                    setState(ConnectorState::FINISHING_NO_CABLE_LOCKED);
                    break;
                case EVSEState::Connected:
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    platform_printfln("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;
    }

    if (tag_deadline != 0 && deadline_elapsed(tag_deadline)) {
        switch (state) {
            case ConnectorState::NO_CABLE_NO_TAG:
            case ConnectorState::AUTH_START_NO_CABLE:
                platform_tag_timed_out(connectorId);
                setState(ConnectorState::FINISHING_NO_CABLE_UNLOCKED);
                break;

            case ConnectorState::NO_TAG:
            case ConnectorState::AUTH_START:
                platform_tag_timed_out(connectorId);
                setState(ConnectorState::FINISHING_UNLOCKED);
                break;

            case ConnectorState::AUTH_START_NO_PLUG:
                platform_tag_timed_out(connectorId);
                setState(ConnectorState::IDLE);
                break;

            case ConnectorState::IDLE:
            case ConnectorState::NO_PLUG:
            case ConnectorState::NO_CABLE:
            case ConnectorState::TRANSACTION:
            case ConnectorState::AUTH_STOP:
            case ConnectorState::FINISHING_UNLOCKED:
            case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
            case ConnectorState::FINISHING_NO_CABLE_LOCKED:
            case ConnectorState::FINISHING_NO_SAME_TAG:
                platform_printfln("Unexpected tag deadline elapsed while Connector is in state %d. Was deadline not cleared?", (int)state);
            break;
        }
    }

    if (cable_deadline != 0 && deadline_elapsed(cable_deadline)) {
        switch (state) {
            case ConnectorState::NO_CABLE_NO_TAG:
            case ConnectorState::AUTH_START_NO_CABLE:
            case ConnectorState::NO_CABLE:
                platform_cable_timed_out(connectorId);
                setState(ConnectorState::FINISHING_NO_CABLE_UNLOCKED);
                break;

            case ConnectorState::AUTH_START_NO_PLUG:
            case ConnectorState::NO_PLUG:
                platform_cable_timed_out(connectorId);
                setState(ConnectorState::IDLE);
                break;


            case ConnectorState::TRANSACTION:
            case ConnectorState::AUTH_STOP:
            case ConnectorState::FINISHING_UNLOCKED:
            case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
            case ConnectorState::FINISHING_NO_CABLE_LOCKED:
            case ConnectorState::FINISHING_NO_SAME_TAG:
                // No transaction is running. Ignore the callback.
                break;

            case ConnectorState::IDLE:
            case ConnectorState::NO_TAG:
            case ConnectorState::AUTH_START:
                platform_printfln("Unexpected tag deadline elapsed while Connector is in state %d. Was deadline not cleared?", (int)state);
            break;
        }
    }

    this->applyState();
    this->sendStatus();
}

// Handles TagSeen, SameTagSeen
void Connector::onTagSeen(const char *tag_id) {
    if (authorized_for.is_same_tag(tag_id)) {
        // Handle same tag.
        switch (state) {
            case ConnectorState::AUTH_START_NO_PLUG:
                setState(ConnectorState::IDLE);
                break;
            case ConnectorState::AUTH_START_NO_CABLE:
                setState(ConnectorState::NO_CABLE_NO_TAG);
                break;
            case ConnectorState::AUTH_START:
                setState(ConnectorState::NO_TAG);
                break;
            case ConnectorState::NO_PLUG:
                setState(ConnectorState::IDLE);
                break;
            case ConnectorState::NO_CABLE:
            case ConnectorState::FINISHING_NO_CABLE_LOCKED:
                setState(ConnectorState::FINISHING_NO_CABLE_UNLOCKED);
                break;
            case ConnectorState::TRANSACTION:
            case ConnectorState::AUTH_STOP:
            case ConnectorState::FINISHING_NO_SAME_TAG:
                this->next_stop_reason = StopTransactionReason::LOCAL;
                setState(ConnectorState::FINISHING_UNLOCKED);
                break;

            case ConnectorState::IDLE:
            case ConnectorState::NO_CABLE_NO_TAG:
            case ConnectorState::NO_TAG:
            case ConnectorState::FINISHING_UNLOCKED:
            case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
                platform_printfln("Unexpected same tag in state %s. Was auth not cleared?", ConnectorState_Strings[(size_t)state]);
                break;
        }
        return;
    }


    // Handle non-same tag.
    switch (state) {
        case ConnectorState::IDLE:
            authorized_for.updateTagId(tag_id);
            setState(ConnectorState::AUTH_START_NO_PLUG);
            break;
        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
            authorized_for.updateTagId(tag_id);
            setState(ConnectorState::AUTH_START_NO_CABLE);
            break;
        case ConnectorState::NO_TAG:
        case ConnectorState::FINISHING_UNLOCKED:
            authorized_for.updateTagId(tag_id);
            setState(ConnectorState::AUTH_START);
            break;

        case ConnectorState::TRANSACTION:
            // We still need the tag ID that started the transaction, so don't override authorized_for here, but send the AUTH request immediately.
            this->sendCallAction(CallAction::AUTHORIZE, Authorize(tag_id));
            setState(ConnectorState::AUTH_STOP);
            break;


        case ConnectorState::AUTH_START_NO_PLUG:
        case ConnectorState::AUTH_START_NO_CABLE:
        case ConnectorState::AUTH_START:
        case ConnectorState::NO_PLUG:
        case ConnectorState::NO_CABLE:
        case ConnectorState::AUTH_STOP:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            platform_printfln("Ignoring other tag in state %s", ConnectorState_Strings[(size_t)state]);
            break;
    }
}

// Handles AuthSuccess, AuthFail
void Connector::onAuthorizeConf(IdTagInfo info) {
    bool auth_success = info.status == ResponseIdTagInfoEntriesStatus::ACCEPTED;
    platform_printfln("%successful auth", auth_success ? "S" : "Uns");

    if (auth_success)
        authorized_for.updateFromIdTagInfo(info);

    // Don't deauth if authorize fails: This could have been an authorize for stopping, so we have to keep the old auth.

    switch (state) {
        case ConnectorState::AUTH_START_NO_PLUG:
            setState(auth_success ? ConnectorState::NO_PLUG : ConnectorState::IDLE);
            break;
        case ConnectorState::AUTH_START_NO_CABLE:
            setState(auth_success ? ConnectorState::NO_CABLE : ConnectorState::NO_CABLE_NO_TAG);
            break;
        case ConnectorState::AUTH_START:
            setState(auth_success ? ConnectorState::TRANSACTION : ConnectorState::NO_TAG);
            break;

        case ConnectorState::AUTH_STOP:
            setState(auth_success ? ConnectorState::FINISHING_UNLOCKED : ConnectorState::TRANSACTION);
            break;

        case ConnectorState::IDLE:
        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::NO_TAG:
        case ConnectorState::NO_PLUG:
        case ConnectorState::NO_CABLE:
        case ConnectorState::TRANSACTION:
        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            platform_printfln("Ignoring Authorize.conf in state %s", ConnectorState_Strings[(size_t)state]);
            return;
    }
}

// Handles StartTxnNotAccepted
void Connector::onStartTransactionConf(IdTagInfo info, int32_t txn_id) {
    // TODO: filter with message ID to make sure we don't act on a StartTxn.conf that was for an old transaction.

    if (state != ConnectorState::TRANSACTION && state != ConnectorState::AUTH_STOP) {
        platform_printfln("Ignoring Authorize.conf in state %s", ConnectorState_Strings[(size_t)state]);
        return;
    }

    authorized_for.updateFromIdTagInfo(info);

    this->transaction_id = txn_id;

    if (info.status == ResponseIdTagInfoEntriesStatus::ACCEPTED) {
        return;
    }

    if (getBoolConfig(ConfigKey::StopTransactionOnInvalidId)) {
        this->next_stop_reason = StopTransactionReason::DE_AUTHORIZED;
        setState(ConnectorState::FINISHING_NO_SAME_TAG);
        this->next_stop_reason = StopTransactionReason::NONE;
        return;
    }

    // TODO: Figure out if the spec really means INVALID here or everything that is not ACCEPTED.
    //if (info.status == ResponseIdTagInfoEntriesStatus::INVALID) {
        this->transaction_with_invalid_tag_id = true;
    //}
}

// Handles StopCallback
void Connector::onStop(StopReason reason)
{
    switch (state) {
        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            switch (reason) {
                case StopReason::EmergencyStop:
                    this->next_stop_reason = StopTransactionReason::EMERGENCY_STOP;
                    break;
                case StopReason::Local:
                    this->next_stop_reason = StopTransactionReason::LOCAL;
                    break;
                case StopReason::Other:
                    this->next_stop_reason = StopTransactionReason::OTHER;
                    break;
                case StopReason::PowerLoss:
                    this->next_stop_reason = StopTransactionReason::POWER_LOSS;
                    break;
                case StopReason::Reboot:
                    this->next_stop_reason = StopTransactionReason::REBOOT;
                    break;
                case StopReason::Remote:
                    this->next_stop_reason = StopTransactionReason::REMOTE;
                    break;
            }
            setState(ConnectorState::FINISHING_NO_SAME_TAG);
            this->next_stop_reason = StopTransactionReason::NONE;
            break;

        case ConnectorState::IDLE:
        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::NO_TAG:
        case ConnectorState::AUTH_START_NO_PLUG:
        case ConnectorState::AUTH_START_NO_CABLE:
        case ConnectorState::AUTH_START:
        case ConnectorState::NO_PLUG:
        case ConnectorState::NO_CABLE:
        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            // No transaction is running. Ignore the callback.
            break;
    }
}
