#include "Connector.h"

#include "Tools.h"
#include "ChargePoint.h"
#include "Persistency.h"
#include "Platform.h"
#include "SignedMeterValueHelper.h"

void Connector::deauth() {
    authorized_for = IdTagInfo{};
    authorized_for_level = ExtOCMFIL::NONE;
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
    "START_TXN",
    "TRANSACTION",
    "AUTH_STOP",
    "STOP_TXN",
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
            platform_set_charging_phases(connectorId, this->phases_allowed);
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
            platform_set_charging_phases(connectorId, this->phases_allowed);
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
            platform_set_charging_phases(connectorId, this->phases_allowed);
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
            platform_set_charging_phases(connectorId, this->phases_allowed);
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
            platform_set_charging_phases(connectorId, this->phases_allowed);
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
            platform_set_charging_phases(connectorId, this->phases_allowed);
            platform_unlock_cable(connectorId);

            clearTagDeadline();
            setCableDeadline();

            transaction_id = INT32_MAX;
            transaction_start_time = 0;
            transaction_confirmed_id = 0;
            transaction_with_non_accepted_tag_id = false;
            break;

        case ConnectorState::START_TXN:
        case ConnectorState::STOP_TXN:
            platform_lock_cable(connectorId);
            platform_set_charging_current(connectorId, 0);
            platform_set_charging_phases(connectorId, this->phases_allowed);

            clearTagDeadline();
            clearCableDeadline();
            break;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            platform_lock_cable(connectorId);
            // TODO: implement MaxEnergyOnInvalidId here
            platform_set_charging_current(connectorId, transaction_with_non_accepted_tag_id ? 0 : this->current_allowed);
            platform_set_charging_phases(connectorId, this->phases_allowed);

            clearTagDeadline();
            clearCableDeadline();
            break;

        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            platform_lock_cable(connectorId);
            platform_set_charging_current(connectorId, 0);
            platform_set_charging_phases(connectorId, this->phases_allowed);

            clearTagDeadline();
            clearCableDeadline();

            transaction_id = INT32_MAX;
            transaction_start_time = 0;
            transaction_confirmed_id = 0;
            transaction_with_non_accepted_tag_id = false;
            break;
    }
}

enum class TransitionAction {
    FORBIDDEN,
    NO_ACTION,
    SEND_AUTHENTICATE,
    NOTIFY_METER_BEGIN,
    SEND_START_TXN,
    NOTIFY_METER_END,
    SEND_STOP_TXN,
    CLEAR_UNAVAILABLE_REQUESTED_FLAG
};

#undef NOP // arduino-esp32 defines NOP()
#define NOP TransitionAction::NO_ACTION
#define _   TransitionAction::FORBIDDEN
#define NIY TransitionAction::NO_ACTION /*not implemented yet*/
#define AUT TransitionAction::SEND_AUTHENTICATE
#define NMB TransitionAction::NOTIFY_METER_BEGIN
#define STX TransitionAction::SEND_START_TXN
#define NME TransitionAction::NOTIFY_METER_END
#define SPX TransitionAction::SEND_STOP_TXN
#define UNA TransitionAction::CLEAR_UNAVAILABLE_REQUESTED_FLAG

static constexpr TransitionAction state_machine[(size_t)ConnectorState::_max + 1][(size_t)ConnectorState::_max + 1] {
/*                                      |       |       |       |       |       |       |       |       |       |       |       |       |   F   |       |       |
                                        |       |       |       |       |       |       |       |       |       |       |       |       |   I   |       |       |
                                        |       |       |       |       |       |       |       |       |       |       |       |       |   N   |   F   |       |
                                        |       |       |       |       |       |       |       |       |       |       |       |       |   I   |   I   |       |
                                        |       |       |       |       |       |       |       |       |       |       |       |       |   S   |   N   |       |
                                        |       |       |       |       |       |       |       |       |       |       |       |       |   H   |   I   |       |
                                        |       |       |       |       |       |       |       |       |       |       |       |       |   I   |   S   |   F   |
                                        |       |       |       |       |       |       |       |       |       |       |       |       |   N   |   H   |   I   |
                                        |       |       |       |   A   |       |       |       |       |       |       |       |       |   G   |   I   |   N   |
                                        |       |       |   A   |   U   |       |       |       |       |       |       |       |   F   |   _   |   N   |   I   |
                                        |       |       |   U   |   T   |       |       |       |       |       |       |       |   I   |   N   |   G   |   S   |
                                        |       |       |   T   |   H   |       |       |       |       |       |       |       |   N   |   O   |   _   |   H   |
                                        |   N   |       |   H   |   _   |       |       |       |       |       |       |       |   I   |   _   |   N   |   I   |
                                        |   O   |       |   _   |   S   |       |       |       |       |       |       |       |   S   |   C   |   O   |   N   |
                                        |   _   |       |   S   |   T   |       |       |       |       |       |       |       |   H   |   A   |   _   |   G   |
                                        |   C   |       |   T   |   A   |       |       |       |       |       |       |       |   I   |   B   |   C   |   _   |
                                        |   A   |       |   A   |   R   |       |       |       |       |   T   |       |       |   N   |   L   |   A   |   N   |   U
                                        |   B   |       |   R   |   T   |   A   |       |       |       |   R   |       |       |   G   |   E   |   B   |   O   |   N
                                        |   L   |       |   T   |   _   |   U   |       |       |   S   |   A   |   A   |       |   _   |   _   |   L   |   _   |   A
                                        |   E   |       |   _   |   N   |   T   |       |   N   |   T   |   N   |   U   |   S   |   U   |   U   |   E   |   S   |   V
                                        |   _   |       |   N   |   O   |   H   |   N   |   O   |   A   |   S   |   T   |   T   |   N   |   N   |   _   |   A   |   A
                                        |   N   |   N   |   O   |   _   |   _   |   O   |   _   |   R   |   A   |   H   |   O   |   L   |   L   |   L   |   M   |   I
                                        |   O   |   O   |   _   |   C   |   S   |   _   |   C   |   T   |   C   |   _   |   P   |   O   |   O   |   O   |   E   |   L
                                    I   |   _   |   _   |   P   |   A   |   T   |   P   |   A   |   _   |   T   |   S   |   _   |   C   |   C   |   C   |   _   |   A
                                    D   |   T   |   T   |   L   |   B   |   A   |   L   |   B   |   T   |   I   |   T   |   T   |   K   |   K   |   K   |   T   |   B
                                    L   |   A   |   A   |   U   |   L   |   R   |   U   |   L   |   X   |   O   |   O   |   X   |   E   |   E   |   E   |   A   |   L
                                    E   |   G   |   G   |   G   |   E   |   T   |   G   |   E   |   N   |   N   |   P   |   N   |   D   |   D   |   D   |   G   |   E
 from                        to:  IDLE  | NC_NT	|NO_TAG	| AS_NP	| AS_NC	|A_START|NO_PLUG|NO_CABL|STA_TXN|  TXN	|A_STOP	|STP_TXN|F_UNLKD|F_NC_UL|F_NC_LC|F_NSTAG|UNAVAIL*/
/*                      IDLE */ {   _   ,  NOP  ,  NOP  ,  AUT  ,   _   ,   _   ,  NOP  ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,  UNA  },

/*           NO_CABLE_NO_TAG */ {  NOP  ,   _   ,  NOP  ,   _   ,  AUT  ,   _   ,   _   ,  NOP  ,   _   ,   _   ,   _   ,   _   ,   _   ,  NOP  ,   _   ,   _   ,  UNA  },
/*                    NO_TAG */ {  NOP  ,  NOP  ,   _   ,   _   ,   _   ,  AUT  ,   _   ,   _   ,  NMB  ,   _   ,   _   ,   _   ,  NOP  ,   _   ,   _   ,   _   ,  UNA  },
/*        AUTH_START_NO_PLUG */ {  NOP  ,   _   ,   _   ,   _   ,  NOP  ,  NOP  ,  NOP  ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,  UNA  },
/*       AUTH_START_NO_CABLE */ {   _   ,  NOP  ,   _   ,  NOP  ,   _   ,  NOP  ,   _   ,  NOP  ,   _   ,   _   ,   _   ,   _   ,   _   ,  NOP  ,   _   ,   _   ,  UNA  },
/*                AUTH_START */ {   _   ,   _   ,  NOP  ,  NOP  ,  NOP  ,   _   ,   _   ,   _   ,  NMB  ,   _   ,   _   ,   _   ,  NOP  ,   _   ,   _   ,   _   ,  UNA  },
/*                   NO_PLUG */ {  NOP  ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,  NOP  ,  NMB  ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,  UNA  },
/*                  NO_CABLE */ {   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,  NOP  ,   _   ,  NMB  ,   _   ,   _   ,   _   ,   _   ,  NOP  ,   _   ,   _   ,  UNA  },

/*                 START_TXN */ {   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,  STX  ,   _   ,  NIY  ,   _   ,   _   ,   _   ,   _   ,   _   },
/*               TRANSACTION */ {   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,  AUT  ,  NME  ,   _   ,   _   ,   _   ,   _   ,   _   },
/*                 AUTH_STOP */ {   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,  NOP  ,   _   ,  NME  ,   _   ,   _   ,   _   ,   _   ,   _   },
/*                  STOP_TXN */ {   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,  SPX  ,  SPX  ,  SPX  ,  SPX  ,   _   },

/*        FINISHING_UNLOCKED */ {  NOP  ,   _   ,   _   ,   _   ,   _   ,  AUT  ,   _   ,   _   ,  NMB  ,   _   ,   _   ,   _   ,   _   ,  NOP  ,   _   ,   _   ,  UNA  },
/*FINISHING_NO_CABLE_UNLOCKED*/ {  NOP  ,   _   ,  NOP  ,   _   ,  AUT  ,   _   ,   _   ,  NOP  ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,  UNA  },
/* FINISHING_NO_CABLE_LOCKED */ {  NOP  ,   _   ,  NOP  ,   _   ,   _   ,   _   ,   _   ,  NOP  ,   _   ,   _   ,   _   ,   _   ,   _   ,  NOP  ,   _   ,   _   ,   _   },
/*     FINISHING_NO_SAME_TAG */ {  NOP  ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,  NMB  ,   _   ,   _   ,   _   ,  NOP  ,   _   ,  NOP  ,   _   ,   _   },

/*               UNAVAILABLE */ {  NOP  ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   ,   _   },
};
#undef NOP
#undef _
#undef NIY
#undef AUT
#undef NMB
#undef STX
#undef NME
#undef SPX
#undef UNA


void Connector::setState(ConnectorState newState) {
    log_debug("C%" PRId32 " %s -> %s", this->connectorId, ConnectorStateStrings[(int)state], ConnectorStateStrings[(int)newState]);
    ConnectorState oldState = state;
    state = newState;

    TransitionAction action = state_machine[(size_t)oldState][(size_t)newState];

    switch (action) {
        case TransitionAction::FORBIDDEN:
            log_error("Unexpected transition from %s to %s!", ConnectorStateStrings[(size_t)oldState], ConnectorStateStrings[(size_t)newState]);
            break;
        case TransitionAction::NO_ACTION:
            break;
        case TransitionAction::SEND_AUTHENTICATE:
            this->sendCallAction(Authorize(tagIdInFlight));
            break;
        case TransitionAction::NOTIFY_METER_BEGIN: {
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

                char gi[42]; // model (20 bytes) + ' ' + vendor (20 bytes) + '\0'
                snprintf(gi, ARRAY_SIZE(gi), "%.20s %.20s", platform_get_charge_point_vendor(), platform_get_charge_point_model());

                bool from_rfid = this->authorized_for_level == ExtOCMFIL::HEARSAY;

                static_assert((size_t) ExtOCMFIFEntry::OCPP_AUTH + 2 == (size_t) ExtOCMFIFEntry::OCPP_AUTH_TLS, "OCPPIdentificationFlag layout not as expected");
                static_assert((size_t) ExtOCMFIFEntry::OCPP_RS + 2 == (size_t) ExtOCMFIFEntry::OCPP_RS_TLS, "OCPPIdentificationFlag layout not as expected");

                char ci[38]; // serial (25 bytes) + ' ' + connector id (11 byte) + '\0'
                snprintf(ci, ARRAY_SIZE(ci), "%.25s %d", platform_get_charge_point_serial_number(), this->connectorId);

                ExtOCMFIFEntry if_[4] {
                    from_rfid ? ExtOCMFIFEntry::RFID_PLAIN : ExtOCMFIFEntry::RFID_NONE,
                    (ExtOCMFIFEntry) ((size_t)(from_rfid ? ExtOCMFIFEntry::OCPP_AUTH : ExtOCMFIFEntry::OCPP_RS)
                                            + (this->cp->connection.encrypted ? 2 : 0)),
                    ExtOCMFIFEntry::ISO15118_NONE,
                    ExtOCMFIFEntry::PLMN_NONE,
                };

                ExtOCMF ocmf {
                    /*PG,*/  "unused",
                    /*MS,*/  "unused",
                    /*IS,*/  true,
                    /*IT,*/  from_rfid ? ExtOCMFIT::ISO14443 : ExtOCMFIT::UNDEFINED,
                    reinterpret_cast<ExtOCMFRD*>(0x00000001), 0, //RD is required (but allowed to be empty). If we pass nullptr RD is not serialized. If we pass any other pointer but size 0, an empty array is serizalized.
                    /*FV,*/  nullptr,
                    /*GI*/   gi,
                    /*GS*/   platform_get_charge_point_serial_number(),
                    /*GV*/   platform_get_firmware_version(),
                    /*MV,*/  nullptr,
                    /*MM,*/  nullptr,
                    /*MF,*/  nullptr,
                    /*IL,*/  this->authorized_for_level,
                    /*IF,*/  if_, 4,
                    /*ID,*/  this->authorized_for.tagId,
                    /*TT,*/  nullptr,
                    /*CF,*/  platform_get_evse_firmware_version(),
                    /*LC,*/  nullptr,
                    /*CT,*/  ExtOCMFCT::CBIDC,
                    /*CI */  ci,
                    // vendor extensions
                    /*WTF_connector_id*/       this->connectorId,
                    /*WTF_unix_time*/          this->transaction_start_time,
                    /*WTF_signature_encoding*/ ExtOCMFWTF_signature_encoding::BASE16
                };

                txnLogAppend(this->connectorId, TxnLogTag::BEGIN, ocmf);

                platform_prepare_meter_for_txn(cp->platform_ctx, &ocmf);
                platform_request_meter_value(cp->platform_ctx,
                                             ocmf.WTF_connector_id,
                                             ocmf.WTF_unix_time,
                                             ocmf.WTF_signature_encoding,
                                             MeterRequest::BEGIN);
            }
            break;
        case TransitionAction::SEND_START_TXN: {
                int energy;
                if (txn_meter_value != nullptr) {
                    energy = this->txn_meter_value_wh;
                } else {
                    energy = platform_get_energy(connectorId);
                    char buf[12];
                    size_t written = snprintf(buf, sizeof(buf), "%d", energy);
                    txnLogAppend(this->connectorId, TxnLogTag::UNSIGNED_MV_START, buf, written);
                }

                StartTransaction msg{connectorId, authorized_for.tagId, energy, this->transaction_start_time};
                log_info("Created StartTransaction.req at connector %" PRId32 " for tag %s at %.3f kWh.", msg.connectorId, msg.idTag, msg.meterStart / 1000.0f);
                txnLogAppend(this->connectorId, TxnLogTag::START_TXN_REQ, msg);

                this->sendCallAction(msg);
            }
            break;
        case TransitionAction::NOTIFY_METER_END: {
                if (this->next_stop_reason == StopTransactionReason::NONE_) {
                    log_warn("Attempting to send stop transaction but next stop reason is none!");
                }

                this->transaction_stop_time = platform_get_system_time(cp->platform_ctx);

                platform_request_meter_value(this->cp->platform_ctx, this->connectorId, transaction_stop_time, ExtOCMFWTF_signature_encoding::BASE16, MeterRequest::END_WITH_BEGIN);
            }
            break;
        case TransitionAction::SEND_STOP_TXN: {
                int energy;
                if (txn_meter_value != nullptr) {
                    energy = this->txn_meter_value_wh;
                } else {
                    energy = platform_get_energy(connectorId);
                    char buf[12];
                    size_t written = snprintf(buf, sizeof(buf), "%d", energy);
                    txnLogAppend(this->connectorId, TxnLogTag::UNSIGNED_MV_STOP, buf, written);
                }

                this->meter_value_handler.onStopTransaction();

                std::unique_ptr<char[]> signed_meter_value;
                MeterValueSampledValue sv;
                MeterValue mv;

                // TODO don't pass tag ID if the stop was not requested with an identifier
                auto msg = SMVH::buildStopTxn(
                    this->connectorId,
                    this->transaction_stop_time,
                    energy,
                    this->transaction_id,
                    authorized_for.tagId,
                    this->next_stop_reason,
                    this->txn_meter_value_encoding_method,
                    this->txn_meter_value_signing_method,
                    this->txn_meter_value.get(),
                    signed_meter_value,
                    sv,
                    mv);

                this->txn_meter_value.reset();

                txnLogAppend(this->connectorId, TxnLogTag::STOP_TXN_REQ, msg);

                this->sendCallAction(msg);
            }
            break;
        case TransitionAction::CLEAR_UNAVAILABLE_REQUESTED_FLAG:
            this->unavailable_requested = false;
            break;
    }

    // Directly go to unavailable if new state allows this.
    if (this->unavailable_requested && state_machine[(size_t)newState][(size_t)ConnectorState::UNAVAILABLE] != TransitionAction::FORBIDDEN) {
         this->setState(ConnectorState::UNAVAILABLE);
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
        case ConnectorState::START_TXN:
        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
        case ConnectorState::STOP_TXN:
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
        case ConnectorState::START_TXN:
        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
        case ConnectorState::STOP_TXN:
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
        case ConnectorState::START_TXN:
        case ConnectorState::STOP_TXN:
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
        case ConnectorState::START_TXN:
            return StatusNotificationStatus::PREPARING;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
        case ConnectorState::STOP_TXN: {
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
    return StatusNotificationStatus::NONE_;
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
                    setState(ConnectorState::START_TXN);
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
                    setState(ConnectorState::START_TXN);
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
                    this->next_state_after_stop_txn = ConnectorState::FINISHING_NO_CABLE_UNLOCKED;
                    setState(ConnectorState::STOP_TXN);
                    break;
                case EVSEState::PlugDetected:
                    this->next_stop_reason = StopTransactionReason::EV_DISCONNECTED;
                    setState(getBoolConfig(ConfigKey::UnlockConnectorOnEVSideDisconnect) ? ConnectorState::FINISHING_NO_CABLE_UNLOCKED : ConnectorState::FINISHING_NO_CABLE_LOCKED);
                    this->next_stop_reason = StopTransactionReason::NONE_;
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
                    this->next_state_after_stop_txn = ConnectorState::FINISHING_NO_CABLE_UNLOCKED;
                    setState(ConnectorState::STOP_TXN);
                    break;
                case EVSEState::PlugDetected:
                    this->next_stop_reason = StopTransactionReason::EV_DISCONNECTED;
                    this->next_state_after_stop_txn = getBoolConfig(ConfigKey::UnlockConnectorOnEVSideDisconnect) ? ConnectorState::FINISHING_NO_CABLE_UNLOCKED : ConnectorState::FINISHING_NO_CABLE_LOCKED;
                    setState(ConnectorState::STOP_TXN);
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
                // TODO: ReadyToCharge and Charging can happen if the EVSE is currently waking up a car. Fix this in all finishing states!
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

        case ConnectorState::START_TXN:
        case ConnectorState::STOP_TXN:
            // Ignore all pollable events in START/STOP_TXN
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
            case ConnectorState::START_TXN:
            case ConnectorState::STOP_TXN:
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
            case ConnectorState::START_TXN:
            case ConnectorState::STOP_TXN:
                log_warn("Unexpected tag deadline elapsed while Connector is in state %d. Was deadline not cleared?", (int)state);
            break;
        }
    }

    if (state == ConnectorState::START_TXN) {
        bool signed_values_enabled = getBoolConfig(ConfigKey::SampledDataSignReadings) || getBoolConfig(ConfigKey::AlignedDataSignReadings);
        bool signed_start_enabled = getCSLConfigLen(ConfigKey::StartTxnSampledData) > 0;
        if (!signed_values_enabled || !signed_start_enabled)
            // No need to wait for signed meter values if they won't be sent.
            setState(ConnectorState::TRANSACTION);
    }

    if (state == ConnectorState::STOP_TXN) {
        bool signed_values_enabled = getBoolConfig(ConfigKey::SampledDataSignReadings) || getBoolConfig(ConfigKey::AlignedDataSignReadings);
        if (!signed_values_enabled) {
            // No need to wait for signed meter values if they won't be sent.
            auto next_state = this->next_state_after_stop_txn;
            this->next_state_after_stop_txn = ConnectorState::IDLE;
            this->setState(next_state);

        }
    }


    // It's fine that we set another state above:
    // There is no need to apply a state that we leave directly afterwards.
    // Also not all state changes have to be sent as StatusNotification.reqs:
    /*
       A Charge Point manufacturer MAY have implemented a minimal status duration for certain
       status transitions separate of the MinimumStatusDuration setting. The time set in
       MinimumStatusDuration will be added to this default delay. Setting
       MinimumStatusDuration to zero SHALL NOT override the default manufacturer’s minimal
       status duration.
    */

    this->applyState();
    this->sendStatus();
    this->meter_value_handler.tick();
#ifdef OCPP_STATE_CALLBACKS
    platform_update_connector_state(connectorId, state, last_sent_status, authorized_for, tag_deadline, cable_deadline, transaction_id, transaction_confirmed_id, transaction_start_time, current_allowed, transaction_with_non_accepted_tag_id, unavailable_requested);
#endif
}

// Handles TagSeen, SameTagSeen
void Connector::onTagSeen(const char *tag_id, bool from_remote_start_txn) {
    if (authorized_for.is_same_tag(tag_id)) {
        platform_tag_accepted(this->connectorId, tag_id);

        // Handle same tag. This will always de-authenticate.
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
                this->next_stop_reason = StopTransactionReason::LOCAL;
                this->next_state_after_stop_txn = ConnectorState::FINISHING_UNLOCKED;
                setState(ConnectorState::STOP_TXN);
                break;

            case ConnectorState::FINISHING_NO_SAME_TAG:
                setState(ConnectorState::FINISHING_UNLOCKED);
                break;

            case ConnectorState::START_TXN:
            case ConnectorState::STOP_TXN:
                log_debug("Ignoring same tag in state %s", ConnectorStateStrings[(size_t)state]);
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

    ConnectorState next_state = this->state;

    // Handle non-same tag.
    switch (state) {
        case ConnectorState::IDLE:
            next_state = ConnectorState::AUTH_START_NO_PLUG;
            break;
        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
            next_state = ConnectorState::AUTH_START_NO_CABLE;
            break;
        case ConnectorState::NO_TAG:
        case ConnectorState::FINISHING_UNLOCKED:
            next_state = ConnectorState::AUTH_START;
            break;

        case ConnectorState::TRANSACTION:
            next_state = ConnectorState::AUTH_STOP;
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
        case ConnectorState::START_TXN:
        case ConnectorState::STOP_TXN:
            log_debug("Ignoring other tag in state %s", ConnectorStateStrings[(size_t)state]);
            return;
    }

    // sanity-check
    auto action = state_machine[(size_t)this->state][(size_t)next_state];
    if (action != TransitionAction::SEND_AUTHENTICATE)
        log_warn("Transition from %s to %s should send authenticate but TransitionAction is %d!", ConnectorStateStrings[(size_t)this->state], ConnectorStateStrings[(size_t)next_state], (int)action);

    memset(tagIdInFlight, 0, ARRAY_SIZE(tagIdInFlight));
    strncpy(tagIdInFlight, tag_id, ARRAY_SIZE(tagIdInFlight) - 1);
    authLevelInFlight = from_remote_start_txn ? ExtOCMFIL::TRUSTED : ExtOCMFIL::HEARSAY;

    setState(next_state);
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
        authorized_for_level = authLevelInFlight;

        memset(tagIdInFlight, 0, ARRAY_SIZE(tagIdInFlight));
        authLevelInFlight = ExtOCMFIL::NONE;

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
            setState(auth_success ? ConnectorState::START_TXN : ConnectorState::NO_TAG);
            break;

        case ConnectorState::AUTH_STOP:
            this->next_state_after_stop_txn = ConnectorState::FINISHING_UNLOCKED;
            setState(auth_success ? ConnectorState::STOP_TXN : ConnectorState::TRANSACTION);
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
        case ConnectorState::START_TXN:
        case ConnectorState::STOP_TXN:
            log_debug("Ignoring Authorize.conf in state %s", ConnectorStateStrings[(size_t)state]);
            return;
    }
}

static constexpr size_t constexpr_strlen(const char *s) {
    return (s == nullptr || s[0] == '\0') ? 0
            : (constexpr_strlen(&s[1]) + 1);
}

// Begin stolen and modified from https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/
// MIT licensed: https://nachtimwald.com/legal/
size_t b64_encoded_size(size_t inlen)
{
	size_t ret;

	ret = inlen;
	if (inlen % 3 != 0)
		ret += 3 - (inlen % 3);
	ret /= 3;
	ret *= 4;

	return ret;
}

const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

bool b64_encode(const char *in, size_t len, char *out, size_t out_len)
{
	size_t  elen;
	size_t  i;
	size_t  j;
	size_t  v;

    elen = b64_encoded_size(len);

	if (in == NULL || len == 0 || out == NULL || out_len < elen + 1)
		return false;

	out[elen] = '\0';

	for (i=0, j=0; i<len; i+=3, j+=4) {
		v = in[i];
		v = i+1 < len ? v << 8 | in[i+1] : v << 8;
		v = i+2 < len ? v << 8 | in[i+2] : v << 8;

		out[j]   = b64chars[(v >> 18) & 0x3F];
		out[j+1] = b64chars[(v >> 12) & 0x3F];
		if (i+1 < len) {
			out[j+2] = b64chars[(v >> 6) & 0x3F];
		} else {
			out[j+2] = '=';
		}
		if (i+2 < len) {
			out[j+3] = b64chars[v & 0x3F];
		} else {
			out[j+3] = '=';
		}
	}

	return out;
}

// End stolen code


std::unique_ptr<char[]> Connector::serializeSignedMeterValue(bool send_public_key) {
    const char *publicKey = nullptr;
    if (send_public_key) {
        static_assert(OCPP_NUM_CONNECTORS == 1, "only one connector is supported for now: each connector requires a separate MeterPublicKey[ConnectorID] ConfigKey");
        publicKey = getStringConfig(ConfigKey::MeterPublicKey1);
    }

    return SMVH::serializeSignedMeterValue(publicKey, this->txn_meter_value_encoding_method, this->txn_meter_value_signing_method, this->txn_meter_value.get());
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

    char buf[12];
    size_t written = snprintf(buf, sizeof(buf), "%d", txn_id);
    txnLogAppend(this->connectorId, TxnLogTag::TXN_ID, buf, written);

    if (this->txn_meter_value != nullptr) {
        std::unique_ptr<char[]> signed_meter_value;
        MeterValueSampledValue sv;
        MeterValue mv;

        auto msg = SMVH::buildTxnStartMeterValues(
            this->connectorId,
            this->transaction_id,
            this->transaction_start_time,
            this->txn_meter_value_encoding_method,
            this->txn_meter_value_signing_method,
            this->txn_meter_value.get(),
            signed_meter_value,
            sv,
            mv);

        this->txn_meter_value.reset();

        this->signed_start_meter_values_message_id = msg.ocppJmessageId;
        txnLogAppend(this->connectorId, TxnLogTag::MV_START_REQ, msg);
        this->sendCallAction(msg);
    }

    if (info.status == ResponseIdTagInfoEntriesStatus::ACCEPTED) {
        return;
    }

    if (getBoolConfig(ConfigKey::StopTransactionOnInvalidId)) {
        this->next_stop_reason = StopTransactionReason::DE_AUTHORIZED;
        this->next_state_after_stop_txn = ConnectorState::FINISHING_NO_SAME_TAG;
        setState(ConnectorState::STOP_TXN);
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

void Connector::onStopTransactionConf(Option<StopTransactionResponseIdTagInfoEntriesView> info) {
    (void) info;

    txnLogRemove(this->connectorId);
}

void Connector::onMeterValuesConf() {
    if (this->signed_start_meter_values_message_id != 0) {
        txnLogAppend(this->connectorId, TxnLogTag::MV_START_CONFIRMED, "null", 4);
        this->signed_start_meter_values_message_id = 0;
    }
}

void Connector::onSignedMeterValue(ExtSMVSignedMeterValueTypeSigningMethod signing_method, ExtSMVSignedMeterValueTypeEncodingMethod encoding_method, char *data /*OCMF container, not base64 encoded*/, size_t data_len) {
    switch (state) {
        case ConnectorState::START_TXN: {
            if (this->txn_meter_value != nullptr)
                log_error("Overwriting potentially unsent txn related signed meter value!");

            {
                auto pr = SMVH::parseOCMF(data, data_len);

                if (pr.is_none()) {
                    log_error("Failed to parse OCMF container for begin");
                    return;
                }

                auto ocmf = pr.unwrap().get_view();

                if (ocmf.RD_count() != 1) {
                    log_error("Expected exactly one reading in OCMF container for begin. Got %zu", ocmf.RD_count());
                    return;
                }

                // TODO check unit/obis here?

                // unwrap is fine here, we've made sure that there is exactly one reading
                this->txn_meter_value_wh = SMVH::getEnergyWhFromOCMF(ocmf, 0).unwrap();
            }
            this->txn_meter_value_encoding_method = encoding_method;
            this->txn_meter_value_signing_method = signing_method;

            txnLogAppend(this->connectorId, TxnLogTag::SIGNED_MV_START, data, data_len);

            auto base64_len = b64_encoded_size(data_len) + 1;
            this->txn_meter_value = heap_alloc_array<char>(base64_len);
            b64_encode(data, data_len, this->txn_meter_value.get(), base64_len);

            this->setState(ConnectorState::TRANSACTION);
            break;
        }
        case ConnectorState::STOP_TXN: {
            if (this->txn_meter_value != nullptr)
                log_error("Overwriting potentially unsent txn related signed meter value!");

            {
                auto pr = SMVH::parseOCMF(data, data_len);

                if (pr.is_none()) {
                    // TODO error handling
                    break;
                }

                auto ocmf = pr.unwrap().get_view();

                if (ocmf.RD_count() != 2)
                    log_error("Expected exactly two readings in OCMF container for end. Got %zu", ocmf.RD_count());

                // TODO check unit/obis here?

                // unwrap is fine here, we've made sure that there are exactly two readings
                this->txn_meter_value_wh = SMVH::getEnergyWhFromOCMF(ocmf, 1).unwrap();
            }

            this->txn_meter_value_encoding_method = encoding_method;
            this->txn_meter_value_signing_method = signing_method;

            txnLogAppend(this->connectorId, TxnLogTag::SIGNED_MV_STOP, data, data_len);

            auto base64_len = b64_encoded_size(data_len) + 1;
            this->txn_meter_value = heap_alloc_array<char>(base64_len);
            b64_encode(data, data_len, this->txn_meter_value.get(), base64_len);

            auto next_state = this->next_state_after_stop_txn;
            this->next_state_after_stop_txn = ConnectorState::IDLE;
            this->setState(next_state);
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
        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
        case ConnectorState::UNAVAILABLE:
            log_debug("Ignoring signed meter value in state %d", (int)state);
            break;
    }
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
        case ConnectorState::START_TXN:
            return false;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
        case ConnectorState::STOP_TXN:
            return true;
    }
    SILENCE_GCC_UNREACHABLE();
}

void Connector::onAuthorizedRemoteStartTransaction(const char *tag_id)
{
    ConnectorState next_state = this->state;

    switch (state) {
        case ConnectorState::IDLE:
        case ConnectorState::AUTH_START_NO_PLUG:
            next_state = ConnectorState::NO_PLUG;
            break;

        case ConnectorState::NO_CABLE_NO_TAG:
        case ConnectorState::AUTH_START_NO_CABLE:
        case ConnectorState::FINISHING_NO_CABLE_UNLOCKED:
        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
            next_state = ConnectorState::NO_CABLE;
            break;

        case ConnectorState::NO_TAG:
        case ConnectorState::AUTH_START:
        case ConnectorState::FINISHING_UNLOCKED:
        case ConnectorState::FINISHING_NO_SAME_TAG:
            next_state = ConnectorState::START_TXN;
            break;

        case ConnectorState::NO_PLUG:
        case ConnectorState::NO_CABLE:
            // stay in this state, only change tag id
            break;

        case ConnectorState::START_TXN:
        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
        case ConnectorState::STOP_TXN:
        case ConnectorState::UNAVAILABLE:
            log_debug("Ignoring remote start transaction in state %s", ConnectorStateStrings[(size_t)state]);
            return;
    }

    authorized_for.updateTagId(tag_id);
    authorized_for_level = ExtOCMFIL::TRUSTED;
    setState(next_state);
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
        case ConnectorState::START_TXN:
        case ConnectorState::STOP_TXN:
            return;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            this->next_stop_reason = StopTransactionReason::REMOTE;
            this->next_state_after_stop_txn = ConnectorState::FINISHING_UNLOCKED;
            setState(ConnectorState::STOP_TXN);
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
        case ConnectorState::START_TXN: // TODO: technically incorrect
        case ConnectorState::STOP_TXN: // TODO: technically incorrect
            return UnlockConnectorResponseStatus::UNLOCKED;

        case ConnectorState::FINISHING_NO_CABLE_LOCKED:
            setState(ConnectorState::FINISHING_NO_CABLE_UNLOCKED);
            return UnlockConnectorResponseStatus::UNLOCKED;

        case ConnectorState::TRANSACTION:
        case ConnectorState::AUTH_STOP:
            this->next_stop_reason = StopTransactionReason::UNLOCK_COMMAND;
            this->next_state_after_stop_txn = ConnectorState::FINISHING_UNLOCKED;
            setState(ConnectorState::STOP_TXN);
            return UnlockConnectorResponseStatus::UNLOCKED;

        case ConnectorState::FINISHING_NO_SAME_TAG:
            setState(ConnectorState::FINISHING_UNLOCKED);
            return UnlockConnectorResponseStatus::UNLOCKED;
    }
    SILENCE_GCC_UNREACHABLE();
}

// Handles ChangeToUnavailable
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
            /* In the event that Central System requests Charge Point to change to a status it is already in, Charge Point SHALL
               respond with availability status ‘Accepted’. */
            if (state == ConnectorState::UNAVAILABLE) {
                return ChangeAvailabilityResponseStatus::ACCEPTED;
            }

            if (state_machine[(size_t)state][(size_t)ConnectorState::UNAVAILABLE] == TransitionAction::FORBIDDEN) {
                // We can't switch to unavailable right now. Schedule for later
                this->unavailable_requested = true;
                return ChangeAvailabilityResponseStatus::SCHEDULED;
            }
            this->setState(ConnectorState::UNAVAILABLE);
            return ChangeAvailabilityResponseStatus::ACCEPTED;
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
            this->next_state_after_stop_txn = ConnectorState::FINISHING_NO_SAME_TAG;
            setState(ConnectorState::STOP_TXN);
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

        case ConnectorState::START_TXN:
        case ConnectorState::STOP_TXN:
            log_error("onStop in START/STOP_TXN currently not implemented!");
            break;
    }
}

void Connector::init(int32_t connId, OcppChargePoint *chargePoint)
{
    this->connectorId = connId;
    this->cp = chargePoint;
    this->meter_value_handler.init(connId, chargePoint);

    char binary[500];
    size_t binary_len = 500;
    binary_len = platform_get_meter_public_key(this->cp->platform_ctx, this->connectorId, binary, binary_len);
    if (binary_len == 0) {
        // TODO
        log_error("Failed to get public key. Add error handling here.");
    }

    char pub_key[500];
    char *ptr = pub_key;

    constexpr const char *prefix = "oca|base64|asn1|";

    constexpr size_t prefix_len = constexpr_strlen(prefix);
    memcpy(ptr, prefix, prefix_len); ptr += prefix_len;

    b64_encode(binary, binary_len, ptr, ARRAY_SIZE(pub_key) - prefix_len);

    static_assert(OCPP_NUM_CONNECTORS == 1, "only one connector is supported for now: each connector requires a separate MeterPublicKey[ConnectorID] ConfigKey");
    getConfig(ConfigKey::MeterPublicKey1).setValue(pub_key, true);

    static_assert(OCPP_NUM_CONNECTORS == 1, "only one connector is supported for now: if this connector does not supported signed meter values, it will deactivate those for all connectors");
    if (!platform_supports_signed_meter_values(this->cp->platform_ctx, this->connectorId)) {
        getConfig(ConfigKey::MeterPublicKey1).hidden = true;
        getConfig(ConfigKey::PublicKeyWithSignedMeterValue).hidden = true;
        getConfig(ConfigKey::SampledDataSignReadings).hidden = true;
        getConfig(ConfigKey::StartTxnSampledData).hidden = true;
        getConfig(ConfigKey::SampledDataSignStartedReadings).hidden = true;
        getConfig(ConfigKey::SampledDataSignUpdatedReadings).hidden = true;
        getConfig(ConfigKey::AlignedDataSignReadings).hidden = true;
        getConfig(ConfigKey::AlignedDataSignUpdatedReadings).hidden = true;
        return;
    }
}
