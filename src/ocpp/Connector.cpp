#include "Connector.h"

#include "Tools.h"
#include "ChargePoint.h"
#include "Persistency.h"
#include "Platform.h"

void Connector::deauth() {
    authorized_for = IdTagInfo{};
}

void Connector::setTagDeadline()
{
    // The spec does not specify explicitly which timeout to use here:
    // The configuration spec only talks about inserting the cable, however
    // transition B9 says "time out (configured by the configuration key: ConnectionTimeOut) on expected user action"
    if (tag_deadline == 0) {
        tag_deadline = set_deadline(getIntConfigUnsigned(ConfigKey::ConnectionTimeOut) * 1000);
        platform_tag_expected(this->connectorId);
    }
}

void Connector::clearTagDeadline()
{
    if (tag_deadline != 0)
        platform_clear_tag_expected(this->connectorId);
    tag_deadline = 0;
}

void Connector::setCableDeadline()
{
    if (cable_deadline == 0)
        cable_deadline = set_deadline(getIntConfigUnsigned(ConfigKey::ConnectionTimeOut) * 1000);
}

void Connector::clearCableDeadline()
{
    cable_deadline = 0;
}

// keep in sync with ocpp_platform_gui
static const char * const ConnectorStateStrings[] = {
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
    "FINISHING_NO_SAME_TAG",
    "UNAVAILABLE"
};

void Connector::applyState() {
    switch (state) {
        case ConnectorState::IDLE:
        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::UNAVAILABLE:
            platform_set_charging_current(connectorId, 0);
            platform_unlock_cable(connectorId);

            clearTagDeadline();
            clearCableDeadline();

            deauth();

            transaction_id = INT32_MAX;
            transaction_start_time = 0;
            transaction_confirmed_id = 0;
            transaction_with_non_accepted_tag_id = false;
            break;

        case ConnectorState::NO_CABLE_NO_TAG:
            platform_set_charging_current(connectorId, 0);
            platform_unlock_cable(connectorId);

            setTagDeadline();
            setCableDeadline();

            deauth();

            transaction_id = INT32_MAX;
            transaction_start_time = 0;
            transaction_confirmed_id = 0;
            transaction_with_non_accepted_tag_id = false;
            break;

        case ConnectorState::NO_TAG:
            platform_set_charging_current(connectorId, 0);
            platform_unlock_cable(connectorId);

            setTagDeadline();
            clearCableDeadline();

            deauth();

            transaction_id = INT32_MAX;
            transaction_start_time = 0;
            transaction_confirmed_id = 0;
            transaction_with_non_accepted_tag_id = false;
            break;

        case ConnectorState::AUTH_START_NO_PLUG:
        case ConnectorState::AUTH_START_NO_CABLE:
            platform_set_charging_current(connectorId, 0);
            platform_unlock_cable(connectorId);

            // We have to set the timeout here: The spec says
            // "Interval *from beginning of status: 'Preparing' until incipient Transaction is automatically canceled, due to failure of EV driver to
            // (correctly) insert the charging cable connector(s) into the appropriate socket(s)."
            setTagDeadline();
            setCableDeadline();

            transaction_id = INT32_MAX;
            transaction_start_time = 0;
            transaction_confirmed_id = 0;
            transaction_with_non_accepted_tag_id = false;
            break;

        case ConnectorState::AUTH_START:
            platform_set_charging_current(connectorId, 0);
            platform_unlock_cable(connectorId);

            setTagDeadline();
            clearCableDeadline();

            transaction_id = INT32_MAX;
            transaction_start_time = 0;
            transaction_confirmed_id = 0;
            transaction_with_non_accepted_tag_id = false;
            break;

        case ConnectorState::NO_PLUG:
        case ConnectorState::NO_CABLE:
            platform_set_charging_current(connectorId, 0);
            platform_unlock_cable(connectorId);

            clearTagDeadline();
            setCableDeadline();

            transaction_id = INT32_MAX;
            transaction_start_time = 0;
            transaction_confirmed_id = 0;
            transaction_with_non_accepted_tag_id = false;
            break;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            platform_lock_cable(connectorId);
            // TODO: implement MaxEnergyOnInvalidId here
            platform_set_charging_current(connectorId, transaction_with_non_accepted_tag_id ? 0 : this->current_allowed);

            clearTagDeadline();
            clearCableDeadline();
            break;

        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            platform_lock_cable(connectorId);
            platform_set_charging_current(connectorId, 0);

            clearTagDeadline();
            clearCableDeadline();

            transaction_id = INT32_MAX;
            transaction_start_time = 0;
            transaction_confirmed_id = 0;
            transaction_with_non_accepted_tag_id = false;
            break;
    }
}

void Connector::setState(ConnectorState newState) {
    log_debug("C%" PRId32 " %s -> %s", this->connectorId, ConnectorStateStrings[(int)state], ConnectorStateStrings[(int)newState]);
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
                    this->sendCallAction(Authorize(tagIdInFlight));
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
                case ConnectorState::UNAVAILABLE:
                    break;
            }
            break;
        case ConnectorState::TRANSACTION:
            switch (oldState) {
                case ConnectorState::AUTH_START:
                case ConnectorState::NO_PLUG:
                case ConnectorState::NO_CABLE:
                case ConnectorState::NO_TAG:
                case ConnectorState::FINISHING_UNLOCKED:
                case ConnectorState::FINISHING_NO_SAME_TAG: {
                    /*
                    If the Charge Point was unable to deliver the StartTransaction.req despite repeated attempts, or if the Central System was
                    unable to deliver the StartTransaction.conf response, then the Charge Point will not receive a transactionId.

                    In that case, the Charge Point SHALL send any Transaction related messages for this transaction to the Central System with a
                    transactionId = -1. The Central System SHALL respond as if these messages refer to a valid transactionId, so that the Charge
                    Point is not blocked by this.
                    */
                    this->transaction_id = -1;
                    this->meter_value_handler.onStartTransaction(this->transaction_id);

                    this->transaction_start_time = platform_get_system_time(cp->platform_ctx);
                    auto energy = platform_get_energy(connectorId);

                    StartTransaction msg{connectorId, authorized_for.tagId, energy, this->transaction_start_time};
                    log_info("Created StartTransaction.req at connector %" PRId32 " for tag %s at %.3f kWh.", msg.connectorId, msg.idTag, msg.meterStart / 1000.0f);

                    persistStartTxn(msg.connectorId, msg.idTag, msg.meterStart, msg.reservationId, (uint32_t)msg.ocppJmessageId, msg.timestamp);

                    this->sendCallAction(msg);
                    break;
                }
                case ConnectorState::AUTH_STOP:
                    break;

                case ConnectorState::IDLE:
                case ConnectorState::NO_CABLE_NO_TAG:
                case ConnectorState::AUTH_START_NO_PLUG:
                case ConnectorState::AUTH_START_NO_CABLE:
                case ConnectorState::TRANSACTION:
                case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
                case ConnectorState::FINISHING_NO_CABLE_LOCKED:
                case ConnectorState::UNAVAILABLE:
                    log_error("Unexpected transition into transaction! We did not send a StartTransaction.req!");
                    break;
            }
            break;

        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
             switch (oldState) {
                case ConnectorState::TRANSACTION:
                case ConnectorState::AUTH_STOP: {
                    if (this->next_stop_reason == StopTransactionReason::NONE) {
                        log_warn("Attempting to send stop transaction but next stop reason is none!");
                    }
                    auto timestamp = platform_get_system_time(cp->platform_ctx);
                    auto energy = platform_get_energy(connectorId);

                    this->meter_value_handler.onStopTransaction();

                    onTxnMsgResponseReceived(this->transaction_confirmed_id);

                    StopTransaction msg{energy, timestamp, transaction_id, authorized_for.tagId, this->next_stop_reason};
                    log_info("Created StopTransaction.req at connector %" PRId32 " for tag %s at %.3f kWh. StopReason %d", this->connectorId, msg.idTag, msg.meterStop / 1000.0f, (int)msg.reason);

                    persistStopTxn((uint8_t)msg.reason, msg.meterStop, msg.transactionId, msg.idTag, (uint32_t) msg.ocppJmessageId, msg.timestamp);

                    this->sendCallAction(msg);
                    break;
                }
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
                case ConnectorState::UNAVAILABLE:
                    break;
            }
            break;

        case ConnectorState::IDLE:
        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::NO_TAG:
        case ConnectorState::NO_PLUG:
        case ConnectorState::NO_CABLE:
        case ConnectorState::AUTH_STOP:
        case ConnectorState::UNAVAILABLE:
            break;
    }

    if (this->unavailable_requested) {
        switch (newState) {
            case ConnectorState::TRANSACTION:
            case ConnectorState::AUTH_STOP:
            case ConnectorState::FINISHING_NO_CABLE_LOCKED:
            case ConnectorState::FINISHING_NO_SAME_TAG:
                break;

            case ConnectorState::IDLE:
            case ConnectorState::NO_CABLE_NO_TAG:
            case ConnectorState::NO_TAG:
            case ConnectorState::AUTH_START_NO_PLUG:
            case ConnectorState::AUTH_START_NO_CABLE:
            case ConnectorState::AUTH_START:
            case ConnectorState::FINISHING_UNLOCKED:
            case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
            case ConnectorState::NO_PLUG:
            case ConnectorState::NO_CABLE:
            case ConnectorState::UNAVAILABLE:
                this->unavailable_requested = false;
                state = ConnectorState::UNAVAILABLE;
        }
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

    this->sendCallAction(StatusNotification(connectorId, StatusNotificationErrorCode::NO_ERROR, newStatus, nullptr, platform_get_system_time(cp->platform_ctx)));
    last_sent_status = newStatus;
}

void Connector::sendCallAction(const ICall &call)
{
    cp->sendCallAction(call, this->connectorId);
}

bool Connector::isSelectableForRemoteStartTxn()
{
    // A connector is selectable if
    // - it is not unavailable or faulted
    // - no transaction is running (we can't start one if one is already running)
    // - at least the plug is detected (as an indicator which connector is selected by the user)

    EVSEState evse_state = platform_get_evse_state(connectorId);
    if (evse_state == EVSEState::Faulted)
        return false;

    switch (state) {
        case ConnectorState::UNAVAILABLE:
        case ConnectorState::IDLE:
        case ConnectorState::NO_PLUG:
        case ConnectorState::AUTH_START_NO_PLUG:
        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            return false;

        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::NO_TAG:
        case ConnectorState::AUTH_START_NO_CABLE: //TODO: How to handle that an authorization is in flight (that could be responded to by the central!)
        case ConnectorState::AUTH_START:
        case ConnectorState::NO_CABLE:
        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            return true;
    }
    SILENCE_GCC_UNREACHABLE();
}

bool Connector::canHandleRemoteStartTxn()
{
    // A connector is can handle a RemoteStartTransaction if
    // - it is not unavailable or faulted
    // - no transaction is running (we can't start one if one is already running)
    // Note that this is not the same as isSelectableForRemoteStartTxn(), as in canHandleRemoteStartTxn
    // we already know, that this connector is the target of the RemoteStartTransaction.req
    // so we don't have to require a plug.

    EVSEState evse_state = platform_get_evse_state(connectorId);
    if (evse_state == EVSEState::Faulted)
        return false;

    switch (state) {
        case ConnectorState::UNAVAILABLE:
        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            return false;

        case ConnectorState::IDLE:
        case ConnectorState::NO_PLUG:
        case ConnectorState::AUTH_START_NO_PLUG:
        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::NO_TAG:
        case ConnectorState::AUTH_START_NO_CABLE: //TODO: How to handle that an authorization is in flight (that could be responded to by the central!)
        case ConnectorState::AUTH_START:
        case ConnectorState::NO_CABLE:
        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            return true;
    }
    SILENCE_GCC_UNREACHABLE();
}

bool Connector::canHandleRemoteStopTxn(int32_t txn_id)
{
    switch (state) {
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
        case ConnectorState::UNAVAILABLE:
            return false;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            return this->transaction_id == txn_id;
    }
    SILENCE_GCC_UNREACHABLE();
}

StatusNotificationStatus Connector::getStatus() {
    EVSEState evse_state = platform_get_evse_state(connectorId);
    if (evse_state == EVSEState::Faulted)
        return StatusNotificationStatus::FAULTED;

    // TODO: implement reserved

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
        case ConnectorState::AUTH_STOP: {
            StatusNotificationStatus result = StatusNotificationStatus::SUSPENDED_EVSE;

            switch (evse_state) {
                case EVSEState::Connected:
                    result = StatusNotificationStatus::SUSPENDED_EVSE;
                    break;
                case EVSEState::ReadyToCharge:
                    result = StatusNotificationStatus::SUSPENDED_EV;
                    break;
                case EVSEState::Charging:
                    result = StatusNotificationStatus::CHARGING;
                    break;
                case EVSEState::PlugDetected: // connector state will transition to finishing* in the next tick
                    result = StatusNotificationStatus::FINISHING;
                    break;
                case EVSEState::NotConnected: // connector state will transition to idle in the next tick
                    result = StatusNotificationStatus::AVAILABLE;
                    break;
                case EVSEState::Faulted:
                    result = StatusNotificationStatus::FAULTED;
                    break;
            }

            // transaction_with_non_accepted_tag_id is set if StartTransaction.conf returns not accepted.
            // In this case report SuspendedEVSE as it has precedence over SuspendedEV or Charging
            if ((result == StatusNotificationStatus::SUSPENDED_EV || result == StatusNotificationStatus::CHARGING) && transaction_with_non_accepted_tag_id)
                return StatusNotificationStatus::SUSPENDED_EVSE;
            return result;
        }

        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            return StatusNotificationStatus::FINISHING;

        case ConnectorState::UNAVAILABLE:
            return StatusNotificationStatus::UNAVAILABLE;
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::NO_PLUG:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    break;
                case EVSEState::PlugDetected:
                    setState(ConnectorState::NO_CABLE);
                    break;
                case EVSEState::Connected:
                    setState(ConnectorState::TRANSACTION);
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::TRANSACTION:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    // This can happen if the charger has a fixed cable, not a plug.
                    this->next_stop_reason = StopTransactionReason::EV_DISCONNECTED;
                    setState(ConnectorState::FINISHING_NO_CABLE_UNLOCKED);
                    this->next_stop_reason = StopTransactionReason::NONE;
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;

        case ConnectorState::AUTH_STOP:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    // This can happen if the charger has a fixed cable, not a plug.
                    this->next_stop_reason = StopTransactionReason::EV_DISCONNECTED;
                    setState(ConnectorState::FINISHING_NO_CABLE_UNLOCKED);
                    this->next_stop_reason = StopTransactionReason::NONE;
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
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
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
                    break;
            }
            break;
        case ConnectorState::UNAVAILABLE:
            switch (evse_state) {
                case EVSEState::NotConnected:
                    break;
                case EVSEState::PlugDetected:
                    // Error here if we lock the connector while unavailable to make sure a plug can not be inserted
                    break;
                case EVSEState::Connected:
                    // Error here if we lock the connector while unavailable to make sure a plug can not be inserted
                    break;
                case EVSEState::ReadyToCharge:
                case EVSEState::Charging:
                case EVSEState::Faulted:
                    log_warn("Unexpected EVSEState %d while Connector is in state %d", (int)evse_state, (int)state);
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
            case ConnectorState::UNAVAILABLE:
                log_warn("Unexpected tag deadline elapsed while Connector is in state %d. Was deadline not cleared?", (int)state);
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
            case ConnectorState::UNAVAILABLE:
                log_warn("Unexpected tag deadline elapsed while Connector is in state %d. Was deadline not cleared?", (int)state);
            break;
        }
    }

    this->applyState();
    this->sendStatus();
    this->meter_value_handler.tick();
#ifdef OCPP_STATE_CALLBACKS
    platform_update_connector_state(connectorId, state, last_sent_status, authorized_for, tag_deadline, cable_deadline, transaction_id, transaction_confirmed_id, transaction_start_time, current_allowed, transaction_with_non_accepted_tag_id, unavailable_requested);
#endif
}

// Handles TagSeen, SameTagSeen
void Connector::onTagSeen(const char *tag_id) {
    if (authorized_for.is_same_tag(tag_id)) {
        platform_tag_accepted(this->connectorId, tag_id);

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
                this->next_stop_reason = StopTransactionReason::NONE;
                break;

            case ConnectorState::IDLE:
            case ConnectorState::NO_CABLE_NO_TAG:
            case ConnectorState::NO_TAG:
            case ConnectorState::FINISHING_UNLOCKED:
            case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
            case ConnectorState::UNAVAILABLE:
                log_warn("Unexpected same tag in state %s. Was auth not cleared?", ConnectorStateStrings[(size_t)state]);
                break;
        }
        return;
    }


    // Handle non-same tag.
    switch (state) {
        case ConnectorState::IDLE:
            memset(tagIdInFlight, 0, ARRAY_SIZE(tagIdInFlight));
            strncpy(tagIdInFlight, tag_id, ARRAY_SIZE(tagIdInFlight) - 1);

            //authorized_for.updateTagId(tag_id);
            setState(ConnectorState::AUTH_START_NO_PLUG);
            break;
        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
            memset(tagIdInFlight, 0, ARRAY_SIZE(tagIdInFlight));
            strncpy(tagIdInFlight, tag_id, ARRAY_SIZE(tagIdInFlight) - 1);

            //authorized_for.updateTagId(tag_id);
            setState(ConnectorState::AUTH_START_NO_CABLE);
            break;
        case ConnectorState::NO_TAG:
        case ConnectorState::FINISHING_UNLOCKED:
            memset(tagIdInFlight, 0, ARRAY_SIZE(tagIdInFlight));
            strncpy(tagIdInFlight, tag_id, ARRAY_SIZE(tagIdInFlight) - 1);

            //authorized_for.updateTagId(tag_id);
            setState(ConnectorState::AUTH_START);
            break;

        case ConnectorState::TRANSACTION:
            memset(tagIdInFlight, 0, ARRAY_SIZE(tagIdInFlight));
            strncpy(tagIdInFlight, tag_id, ARRAY_SIZE(tagIdInFlight) - 1);

            // We still need the tag ID that started the transaction, so don't override authorized_for here, but send the AUTH request immediately.
            this->sendCallAction(Authorize(tag_id));
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
        case ConnectorState::UNAVAILABLE:
            log_debug("Ignoring other tag in state %s", ConnectorStateStrings[(size_t)state]);
            break;
    }
}

void Connector::onAuthorizeError() {
    this->onAuthorizeConf(IdTagInfo{});
}

static TagRejectionType statusToTRT(ResponseIdTagInfoEntriesStatus status) {
    switch (status) {
        case ResponseIdTagInfoEntriesStatus::ACCEPTED:
            log_error("Cant convert ResponseIdTagInfoEntriesStatus::ACCEPTED to TagRejectionType!");
            return TagRejectionType::Invalid;
        case ResponseIdTagInfoEntriesStatus::BLOCKED:
            return TagRejectionType::Blocked;
        case ResponseIdTagInfoEntriesStatus::CONCURRENT_TX:
            return TagRejectionType::ConcurrentTx;
        case ResponseIdTagInfoEntriesStatus::EXPIRED:
            return TagRejectionType::Expired;
        case ResponseIdTagInfoEntriesStatus::INVALID:
            return TagRejectionType::Invalid;
    }
    SILENCE_GCC_UNREACHABLE();
}

// Handles AuthSuccess, AuthFail
void Connector::onAuthorizeConf(IdTagInfo info) {
    bool auth_success = info.status == ResponseIdTagInfoEntriesStatus::ACCEPTED;
    log_info("%successful auth", auth_success ? "S" : "Uns");

    if (auth_success) {
        platform_tag_accepted(this->connectorId, tagIdInFlight);
        authorized_for.updateTagId(tagIdInFlight);
        memset(tagIdInFlight, 0, ARRAY_SIZE(tagIdInFlight));
        authorized_for.updateFromIdTagInfo(info);
    } else {
        platform_tag_rejected(this->connectorId, tagIdInFlight, statusToTRT(info.status));
    }

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
        case ConnectorState::UNAVAILABLE:
            log_debug("Ignoring Authorize.conf in state %s", ConnectorStateStrings[(size_t)state]);
            return;
    }
}

// Handles StartTxnNotAccepted
void Connector::onStartTransactionConf(IdTagInfo info, int32_t txn_id) {
    if (state != ConnectorState::TRANSACTION && state != ConnectorState::AUTH_STOP) {
        log_debug("Ignoring StartTransaction.conf in state %s", ConnectorStateStrings[(size_t)state]);
        return;
    }

    authorized_for.updateFromIdTagInfo(info);

    this->transaction_id = txn_id;
    this->meter_value_handler.transactionId = txn_id;

    this->transaction_confirmed_id = next_call_id++;
    persistRunningTxn(this->connectorId, transaction_confirmed_id, txn_id);

    if (info.status == ResponseIdTagInfoEntriesStatus::ACCEPTED) {
        return;
    }

    if (getBoolConfig(ConfigKey::StopTransactionOnInvalidId)) {
        this->next_stop_reason = StopTransactionReason::DE_AUTHORIZED;
        setState(ConnectorState::FINISHING_NO_SAME_TAG);
        this->next_stop_reason = StopTransactionReason::NONE;
        return;
    }


    // The spec (and errata) talk about how to handle StartTransaction.confs with the authorization status
    // not being "Accepted". Specifically in the note, the spec technically switches to
    // "Invalid" only:
    /*
    Note: In the case of an invalid identifier, an operator MAY choose to
    charge the EV with a minimum
    amount of energy so the EV is able to drive away. This amount is
    controlled by the optional
    configuration key: MaxEnergyOnInvalidId."
    */
    // As it does not make any sense to prefer an invalid tag over (for example) a known, but expired tag
    // we interpret this as an error in the spec and handle all non-"Accepted" statuses in the same way.
    this->transaction_with_non_accepted_tag_id = true;
}

bool Connector::isTransactionActive()
{
    switch (state) {
        case ConnectorState::IDLE:
        case ConnectorState::AUTH_START_NO_PLUG:
        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::AUTH_START_NO_CABLE:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::NO_TAG:
        case ConnectorState::AUTH_START:
        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
        case ConnectorState::NO_PLUG:
        case ConnectorState::NO_CABLE:
        case ConnectorState::UNAVAILABLE:
            return false;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            return true;
    }
    SILENCE_GCC_UNREACHABLE();
}

void Connector::onRemoteStartTransaction(const char *tag_id)
{
    switch (state) {
        case ConnectorState::IDLE:
        case ConnectorState::AUTH_START_NO_PLUG:
            authorized_for.updateTagId(tag_id);
            setState(ConnectorState::NO_PLUG);
            break;

        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::AUTH_START_NO_CABLE:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
            authorized_for.updateTagId(tag_id);
            setState(ConnectorState::NO_CABLE);
            break;

        case ConnectorState::NO_TAG:
        case ConnectorState::AUTH_START:
        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            authorized_for.updateTagId(tag_id);
            setState(ConnectorState::TRANSACTION);
            break;

        case ConnectorState::NO_PLUG:
        case ConnectorState::NO_CABLE:
            authorized_for.updateTagId(tag_id);
            // stay in this state, only change tag id
            break;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
        case ConnectorState::UNAVAILABLE:
            log_debug("Ignoring remote start transaction in state %s", ConnectorStateStrings[(size_t)state]);
            break;
    }
}

void Connector::onRemoteStopTransaction()
{
    switch (state) {
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
        case ConnectorState::UNAVAILABLE:
            return;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            this->next_stop_reason = StopTransactionReason::REMOTE;
            setState(ConnectorState::FINISHING_UNLOCKED);
            this->next_stop_reason = StopTransactionReason::NONE;
    }
}

UnlockConnectorResponseStatus Connector::onUnlockConnector()
{
    switch (state) {
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
        case ConnectorState::UNAVAILABLE:
            return UnlockConnectorResponseStatus::UNLOCKED;

        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
            setState(ConnectorState::FINISHING_NO_CABLE_UNLOCKED);
            return UnlockConnectorResponseStatus::UNLOCKED;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            this->next_stop_reason = StopTransactionReason::UNLOCK_COMMAND;
            setState(ConnectorState::FINISHING_UNLOCKED);
            this->next_stop_reason = StopTransactionReason::NONE;
            return UnlockConnectorResponseStatus::UNLOCKED;
    }
    SILENCE_GCC_UNREACHABLE();
}

ChangeAvailabilityResponseStatus Connector::onChangeAvailability(ChangeAvailabilityType type) {
    switch(type) {
        case ChangeAvailabilityType::OPERATIVE:
            /* In the event that Central System requests Charge Point to change to a status it is already in, Charge Point SHALL
               respond with availability status ‘Accepted’. */
            // Currently there is no reason to not accept this.
            // If in the future a connector can transition to UNAVAILABLE on its own, check here if the reason for this transition was fixed.
            this->unavailable_requested = false;
            if (this->state == ConnectorState::UNAVAILABLE)
                this->setState(ConnectorState::IDLE);
            return ChangeAvailabilityResponseStatus::ACCEPTED;

        case ChangeAvailabilityType::INOPERATIVE:
            switch (state) {
                case ConnectorState::TRANSACTION:
                case ConnectorState::AUTH_STOP:
                    this->unavailable_requested = true;
                    return ChangeAvailabilityResponseStatus::SCHEDULED;

                case ConnectorState::FINISHING_NO_CABLE_LOCKED:
                case ConnectorState::FINISHING_NO_SAME_TAG:
                    this->unavailable_requested = true;
                    return ChangeAvailabilityResponseStatus::ACCEPTED;

                case ConnectorState::UNAVAILABLE:
                /* In the event that Central System requests Charge Point to change to a status it is already in, Charge Point SHALL
                   respond with availability status ‘Accepted’. */
                    this->unavailable_requested = false;
                    return ChangeAvailabilityResponseStatus::ACCEPTED;

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
                    this->unavailable_requested = false;
                    this->setState(ConnectorState::UNAVAILABLE);
                    return ChangeAvailabilityResponseStatus::ACCEPTED;
            }
    }
    SILENCE_GCC_UNREACHABLE();
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
        case ConnectorState::UNAVAILABLE:
            // No transaction is running. Ignore the callback.
            break;
    }
}

void Connector::init(int32_t connId, OcppChargePoint *chargePoint)
{
    this->connectorId = connId;
    this->cp = chargePoint;
    this->meter_value_handler.init(connId, chargePoint);
}
