#include "ChargePoint21.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <common/Platform.h>
#include <common/Tools.h>

// TODO: derive from the platform/EVSE configuration. A single EVSE with
// one connector is enough for now.
#define OCPP21_NUM_EVSES 1

#define OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S 30

namespace Ocpp21 {

static void generate_transaction_id(char buf[OCPP21_TRANSACTION_ID_LEN + 1])
{
    // Pseudo UUIDv4. Not cryptographic, only collision avoidance across
    // reboots is needed.
    uint64_t r1 = ((uint64_t)rand() << 32) ^ (uint64_t)rand() ^ platform_now_ms();
    uint64_t r2 = ((uint64_t)rand() << 32) ^ (uint64_t)rand();
    snprintf(buf, OCPP21_TRANSACTION_ID_LEN + 1, "%08x-%04x-4%03x-8%03x-%012llx",
             (uint32_t)(r1 >> 32),
             (uint32_t)(r1 >> 16) & 0xffff,
             (uint32_t)r1 & 0xfff,
             (uint32_t)(r2 >> 48) & 0xfff,
             (unsigned long long)(r2 & 0xffffffffffffull));
}

bool ChargePoint::start(const char *websocket_endpoint_url, const char *charge_point_name, const char *basic_auth_pass, int32_t security_profile, const PlatformTlsConfig *tls)
{
    this->charge_point_name = charge_point_name;
    device_model.identity = this->charge_point_name.c_str();
    device_model.security_profile = security_profile;
    device_model.cert_store = &cert_store;

    // A persisted password (A01 update) overrides the configured one.
    loadSecurityPersistence();
    if (device_model.basic_auth_password[0] == '\0' && basic_auth_pass != nullptr) {
        snprintf(device_model.basic_auth_password, sizeof(device_model.basic_auth_password), "%s", basic_auth_pass);
    }

    cert_store.init(charge_point_name);
    loadNetworkPersistence();

    // A05: an active network profile overrides the configured endpoint at
    // boot. Falls back to the configured endpoint if the profile needs a
    // CSMS root certificate that is no longer installed.
    const char *endpoint = websocket_endpoint_url;
    const char *effective_pass = device_model.basic_auth_password;
    bool use_tls = tls != nullptr;
    bool profile_boot = device_model.network_priority != 0;
    if (profile_boot) {
        auto &slot = device_model.network_profiles[device_model.network_priority - 1];
        bool have_ca = (tls != nullptr && tls->ca_cert_file != nullptr) || cert_store.find(CertGroup::CsmsRoot) != nullptr;
        if (slot.security_profile >= 2 && !have_ca) {
            log_error("Network profile slot %d needs a CSMS root certificate, using the configured endpoint", device_model.network_priority);
            profile_boot = false;
        }
    }
    if (profile_boot) {
        auto &slot = device_model.network_profiles[device_model.network_priority - 1];
        log_info("Using network profile slot %d (security profile %d)", device_model.network_priority, slot.security_profile);
        endpoint = slot.url;
        device_model.security_profile = slot.security_profile;
        if (slot.password[0] != '\0') {
            effective_pass = slot.password;
        }
        use_tls = slot.security_profile >= 2;
    }

    // A certificate installed via A02 overrides a configured client
    // certificate at boot, mirroring the password behavior.
    PlatformTlsConfig effective_tls;
    if (tls != nullptr) {
        if (tls->ca_cert_file != nullptr) {
            tls_ca_file = tls->ca_cert_file;
        }
        if (tls->client_cert_file != nullptr) {
            tls_client_cert_file = tls->client_cert_file;
        }
        if (tls->client_key_file != nullptr) {
            tls_client_key_file = tls->client_key_file;
        }
    }
    if (use_tls) {
        tls_in_use = true;
        if (tls_ca_file.empty()) {
            const CertEntry *root = cert_store.find(CertGroup::CsmsRoot);
            if (root != nullptr) {
                tls_ca_file = cert_store.pemPath(CertGroup::CsmsRoot, root->id);
            }
            // Both empty: the platform decides whether it can verify the
            // server against a system trust store or refuses the URL.
        }

        const CertEntry *client_chain = cert_store.find(CertGroup::CsmsClientChain);
        if (client_chain != nullptr) {
            log_info("Using the charging station certificate installed via OCPP");
            tls_client_cert_file = cert_store.pemPath(CertGroup::CsmsClientChain, client_chain->id);
            tls_client_key_file = cert_store.keyPath(client_chain->id);
        }

        effective_tls.ca_cert_file = tls_ca_file.empty() ? nullptr : tls_ca_file.c_str();
        effective_tls.client_cert_file = tls_client_cert_file.empty() ? nullptr : tls_client_cert_file.c_str();
        effective_tls.client_key_file = tls_client_key_file.empty() ? nullptr : tls_client_key_file.c_str();
    }

    if (connection.start(endpoint, charge_point_name, effective_pass, use_tls ? &effective_tls : nullptr, this) == nullptr) {
        return false;
    }

    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
        platform_set_charging_allowed21(connection.platform_ctx, evse_id, false);
    }

    // M06: refresh the OCSP status of stored SECC chains after boot.
    for (auto &e : cert_store.all()) {
        if (e.group == CertGroup::V2GChain || e.group == CertGroup::V2G20Chain) {
            scheduleChainOcsp(e.id);
        }
    }

    platform_register_tag_seen_callback21(connection.platform_ctx, [](int32_t evse_id, const char *tag_id, void *user_data){
        ((ChargePoint*)user_data)->onTagSeen(evse_id, tag_id);
    }, this);

    platform_register_stop_callback21(connection.platform_ctx, [](int32_t evse_id, StopReason21 reason, void *user_data){
        ((ChargePoint*)user_data)->onStop(evse_id, reason);
    }, this);

    platform_ws_register_connection_error_callback(connection.platform_ctx, [](PlatformConnectionError error, void *user_data){
        ((ChargePoint*)user_data)->onConnectionError(error);
    }, this);

    boot_retry_deadline = set_deadline(0);
    cert_expiry_check_deadline = set_deadline(0);
    return true;
}

void ChargePoint::stop()
{
    connection.stop();
}

void ChargePoint::tick()
{
    connection.tick();

    // A01: the SetVariablesResponse had time to leave, switch to the new
    // password now.
    if (password_reconnect_deadline != 0 && deadline_elapsed(password_reconnect_deadline)) {
        password_reconnect_deadline = 0;
        connection.updateBasicAuthPassword(device_model.basic_auth_password);
    }

    // A02.FR.08: the CertificateSignedResponse had time to leave,
    // reconnect with the new charging station certificate.
    if (cert_reconnect_deadline != 0 && deadline_elapsed(cert_reconnect_deadline)) {
        cert_reconnect_deadline = 0;
        applyClientCertificate(pending_client_chain_id);
    }

    // A05: the SetVariablesResponse had time to leave, switch to the
    // active network profile now.
    if (network_reconnect_deadline != 0 && deadline_elapsed(network_reconnect_deadline)) {
        network_reconnect_deadline = 0;
        applyNetworkProfile();
    }

    // Transactions continue while offline, their events are queued and
    // flushed after the reconnect.
    if (state == State::Idle)
        tickEvses();

    if (reset_pending)
        tickReset();

    if (!platform_ws_connected(connection.platform_ctx))
        return;

    if (report_active && !report_in_flight)
        sendReportChunk();

    switch (state) {
        case State::PowerOn:
        case State::Pending:
        case State::Rejected:
            if ((deadline_elapsed(boot_retry_deadline) || trigger_boot_notification) && !boot_notification_in_flight) {
                trigger_boot_notification = false;
                sendBootNotification();
            }
            break;

        case State::Idle:
            if (status_notifications_pending || trigger_status_notification) {
                status_notifications_pending = false;
                trigger_status_notification = false;
                sendStatusNotifications();
            }

            if (deadline_elapsed(next_heartbeat_deadline) || trigger_heartbeat) {
                trigger_heartbeat = false;
                next_heartbeat_deadline = set_deadline((uint32_t)device_model.heartbeat_interval_s * 1000);
                connection.sendCallAction(Heartbeat{});
            }

            tickCertificates();
            break;
    }
}

void ChargePoint::onConnect()
{
    log_info("Connected (subprotocol ocpp2.1)");
    last_reported_conn_error = PlatformConnectionError::Unknown;
    if (state != State::Idle)
        boot_retry_deadline = set_deadline(0);
    else
        status_notifications_pending = true;
    // A03: check for soon expiring certificates shortly after each
    // connect, in addition to the periodic check.
    cert_expiry_check_deadline = set_deadline(5000);
}

void ChargePoint::onDisconnect()
{
    log_info("Disconnected");
    boot_notification_in_flight = false;
    abortReport();
    if (ev_cert_in_flight) {
        ev_cert_in_flight = false;
        if (ev_cert_result_cb != nullptr)
            ev_cert_result_cb(false, nullptr, 0, ev_cert_result_user_data);
    }
    if (vehicle_status_in_flight) {
        vehicle_status_in_flight = false;
        platform_vehicle_chain_status_result21(connection.platform_ctx, false);
    }
}

void ChargePoint::onConnectionError(PlatformConnectionError error)
{
    const char *type;
    switch (error) {
        case PlatformConnectionError::InvalidCsmsCertificate: type = "InvalidCsmsCertificate"; break;
        case PlatformConnectionError::InvalidTlsVersion:      type = "InvalidTLSVersion"; break;
        case PlatformConnectionError::InvalidTlsCipherSuite:  type = "InvalidTLSCipherSuite"; break;
        default: return;
    }

    // The connection is terminated by the failed handshake. Queue the
    // security event once per failure streak, it is delivered when a
    // connection is established again (A04.FR.02).
    if (last_reported_conn_error == error)
        return;
    last_reported_conn_error = error;

    log_warn("TLS connection failed: %s", type);
    sendSecurityEventNotification(type);
}

void ChargePoint::sendSecurityEventNotification(const char *type, const char *tech_info)
{
    connection.sendTransactionCallAction(SecurityEventNotification{type, platform_get_system_time(connection.platform_ctx), tech_info});
}

#define OCPP21_SECURITY_PERSISTENCE_BUF_LEN 1024

void ChargePoint::loadSecurityPersistence()
{
    std::string name = charge_point_name + ".sec21";
    auto buf = heap_alloc_array<char>(OCPP21_SECURITY_PERSISTENCE_BUF_LEN);
    size_t len = platform_read_file(name.c_str(), buf.get(), OCPP21_SECURITY_PERSISTENCE_BUF_LEN - 1);
    if (len == 0)
        return;
    buf[len] = '\0';

    StaticJsonDocument<OCPP21_SECURITY_PERSISTENCE_BUF_LEN * 2> doc;
    if (deserializeJson(doc, buf.get(), len)) {
        log_error("Failed to parse %s", name.c_str());
        return;
    }

    if (doc["basic_auth_password"].is<const char *>()) {
        snprintf(device_model.basic_auth_password, sizeof(device_model.basic_auth_password), "%s", doc["basic_auth_password"].as<const char *>());
    }
    if (doc["organization_name"].is<const char *>()) {
        snprintf(device_model.organization_name, sizeof(device_model.organization_name), "%s", doc["organization_name"].as<const char *>());
    }
    if (doc["secc_id"].is<const char *>()) {
        snprintf(device_model.secc_id, sizeof(device_model.secc_id), "%s", doc["secc_id"].as<const char *>());
    }
    if (doc["country_name"].is<const char *>()) {
        snprintf(device_model.country_name, sizeof(device_model.country_name), "%s", doc["country_name"].as<const char *>());
    }
    if (doc["iso_organization_name"].is<const char *>()) {
        snprintf(device_model.iso_organization_name, sizeof(device_model.iso_organization_name), "%s", doc["iso_organization_name"].as<const char *>());
    }
    if (doc["v2g20_use_ed448"].is<bool>()) {
        device_model.v2g20_use_ed448 = doc["v2g20_use_ed448"].as<bool>();
    }
    if (doc["iso15118_enabled"].is<bool>()) {
        device_model.iso15118_enabled = doc["iso15118_enabled"].as<bool>();
    }
    if (doc["v2g_cert_install_enabled"].is<bool>()) {
        device_model.v2g_cert_install_enabled = doc["v2g_cert_install_enabled"].as<bool>();
    }
    if (doc["contract_cert_install_enabled"].is<bool>()) {
        device_model.contract_cert_install_enabled = doc["contract_cert_install_enabled"].as<bool>();
    }
    if (doc["iso15118_evse_id"].is<const char *>()) {
        snprintf(device_model.iso15118_evse_id, sizeof(device_model.iso15118_evse_id), "%s", doc["iso15118_evse_id"].as<const char *>());
    }
    if (doc["enforce_tls_enabled"].is<bool>()) {
        device_model.enforce_tls_enabled = doc["enforce_tls_enabled"].as<bool>();
    }
    if (doc["private_environment_enabled"].is<bool>()) {
        device_model.private_environment_enabled = doc["private_environment_enabled"].as<bool>();
    }
    if (doc["pwm_charging_fallback_timeout_s"].is<int32_t>()) {
        device_model.pwm_charging_fallback_timeout_s = doc["pwm_charging_fallback_timeout_s"].as<int32_t>();
    }
}

void ChargePoint::saveSecurityPersistence()
{
    std::string name = charge_point_name + ".sec21";
    auto buf = heap_alloc_array<char>(OCPP21_SECURITY_PERSISTENCE_BUF_LEN);
    TFJsonSerializer json{buf.get(), OCPP21_SECURITY_PERSISTENCE_BUF_LEN};
    json.addObject();
    json.addMemberString("basic_auth_password", device_model.basic_auth_password);
    json.addMemberString("organization_name", device_model.organization_name);
    json.addMemberString("secc_id", device_model.secc_id);
    json.addMemberString("country_name", device_model.country_name);
    json.addMemberString("iso_organization_name", device_model.iso_organization_name);
    json.addMemberBoolean("v2g20_use_ed448", device_model.v2g20_use_ed448);
    json.addMemberBoolean("iso15118_enabled", device_model.iso15118_enabled);
    json.addMemberBoolean("v2g_cert_install_enabled", device_model.v2g_cert_install_enabled);
    json.addMemberBoolean("contract_cert_install_enabled", device_model.contract_cert_install_enabled);
    json.addMemberString("iso15118_evse_id", device_model.iso15118_evse_id);
    json.addMemberBoolean("enforce_tls_enabled", device_model.enforce_tls_enabled);
    json.addMemberBoolean("private_environment_enabled", device_model.private_environment_enabled);
    json.addMemberNumber("pwm_charging_fallback_timeout_s", device_model.pwm_charging_fallback_timeout_s);
    json.endObject();
    size_t len = json.end();

    if (!platform_write_file(name.c_str(), buf.get(), len)) {
        log_error("Failed to write %s", name.c_str());
    }
}

void ChargePoint::onTimeout(CallAction action, uint64_t messageId)
{
    (void)messageId;
    if (action == CallAction::BOOT_NOTIFICATION) {
        boot_notification_in_flight = false;
        boot_retry_deadline = set_deadline(OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S * 1000);
    }
    if (action == CallAction::AUTHORIZE) {
        log_info("Authorize timed out");
        authorize_in_flight = false;
        authorize_for_stop = false;
        if (authorize_evse_id >= 1 && authorize_evse_id <= OCPP21_NUM_EVSES)
            platform_tag_rejected21(connection.platform_ctx, authorize_evse_id, authorize_token, TagRejectionType21::Invalid);
    }
    if (action == CallAction::GET_CERTIFICATE_STATUS && ocsp_in_flight_idx >= 0) {
        auto &slot = ocsp_cache[ocsp_in_flight_idx];
        slot.in_flight = false;
        slot.refresh_deadline = set_deadline(10 * 60 * 1000);
        ocsp_in_flight_idx = -1;
    }
    if (action == CallAction::NOTIFY_REPORT)
        abortReport();
    if (action == CallAction::GET15118_EV_CERTIFICATE && ev_cert_in_flight) {
        ev_cert_in_flight = false;
        log_warn("Get15118EVCertificate failed");
        if (ev_cert_result_cb != nullptr)
            ev_cert_result_cb(false, nullptr, 0, ev_cert_result_user_data);
    }
    if (action == CallAction::GET_CERTIFICATE_CHAIN_STATUS && vehicle_status_in_flight) {
        vehicle_status_in_flight = false;
        log_warn("GetCertificateChainStatus failed");
        platform_vehicle_chain_status_result21(connection.platform_ctx, false);
    }
}

void ChargePoint::onCallError(CallAction action, uint64_t messageId)
{
    this->onTimeout(action, messageId);
}

void ChargePoint::sendBootNotification()
{
    BootNotificationChargingStation cs;
    cs.model = platform_get_charge_point_model();
    cs.vendorName = platform_get_charge_point_vendor();
    cs.serialNumber = platform_get_charge_point_serial_number();
    cs.firmwareVersion = platform_get_firmware_version();

    boot_notification_in_flight = connection.sendCallAction(BootNotification{&cs, BootNotificationReason::POWER_UP});
}

void ChargePoint::onTagSeen(int32_t evse_id, const char *tag_id)
{
    if (tag_pending) {
        log_info("Tag %s seen while a tag is still being processed. Ignored", tag_id);
        return;
    }
    tag_evse_id = evse_id;
    strncpy(pending_tag, tag_id, OCPP21_ID_TOKEN_LEN);
    pending_tag[OCPP21_ID_TOKEN_LEN] = '\0';
    tag_pending = true;
}

void ChargePoint::onStop(int32_t evse_id, StopReason21 reason)
{
    if (stop_pending) {
        log_info("Stop reported while a stop is still being processed. Ignored");
        return;
    }
    stop_evse_id = evse_id;
    stop_reason = reason;
    stop_pending = true;
}

static StatusNotificationConnectorStatus connector_status(EvseState21 s, const EvseTracker &t)
{
    if (s == EvseState21::Faulted)
        return StatusNotificationConnectorStatus::FAULTED;
    if (s != EvseState21::NotConnected || t.transaction_active || t.authorized)
        return StatusNotificationConnectorStatus::OCCUPIED;
    return StatusNotificationConnectorStatus::AVAILABLE;
}

static bool evse_plugged(EvseState21 s)
{
    return s == EvseState21::Connected || s == EvseState21::ReadyToCharge || s == EvseState21::Charging;
}

static TransactionEventTransactionInfoChargingState charging_state_for(EvseState21 s)
{
    switch (s) {
        case EvseState21::Charging:      return TransactionEventTransactionInfoChargingState::CHARGING;
        case EvseState21::ReadyToCharge: return TransactionEventTransactionInfoChargingState::SUSPENDED_EV;
        default:                         return TransactionEventTransactionInfoChargingState::SUSPENDED_EVSE;
    }
}

static TagRejectionType21 rejection_type(ResponseIdTokenInfoEntriesStatus status)
{
    switch (status) {
        case ResponseIdTokenInfoEntriesStatus::BLOCKED:       return TagRejectionType21::Blocked;
        case ResponseIdTokenInfoEntriesStatus::EXPIRED:       return TagRejectionType21::Expired;
        case ResponseIdTokenInfoEntriesStatus::CONCURRENT_TX: return TagRejectionType21::ConcurrentTx;
        default:                                              return TagRejectionType21::Invalid;
    }
}

static TransactionEventTransactionInfoStoppedReason stopped_reason_for(StopReason21 reason)
{
    switch (reason) {
        case StopReason21::EmergencyStop: return TransactionEventTransactionInfoStoppedReason::EMERGENCY_STOP;
        case StopReason21::Local:         return TransactionEventTransactionInfoStoppedReason::LOCAL;
        case StopReason21::PowerLoss:     return TransactionEventTransactionInfoStoppedReason::POWER_LOSS;
        case StopReason21::Reboot:        return TransactionEventTransactionInfoStoppedReason::REBOOT;
        case StopReason21::Remote:        return TransactionEventTransactionInfoStoppedReason::REMOTE;
        default:                          return TransactionEventTransactionInfoStoppedReason::OTHER;
    }
}

void ChargePoint::sendStatusNotifications()
{
    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
        auto &t = evses[evse_id - 1];
        auto status = connector_status(platform_get_evse_state21(connection.platform_ctx, evse_id), t);
        t.last_sent_status = status;
        connection.sendCallAction(StatusNotification{
            platform_get_system_time(connection.platform_ctx),
            status,
            evse_id,
            1});
    }
}

void ChargePoint::tickEvses()
{
    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
        auto &t = evses[evse_id - 1];
        EvseState21 s = platform_get_evse_state21(connection.platform_ctx, evse_id);
        bool plugged = evse_plugged(s);

        if (tag_pending && (tag_evse_id == 0 || tag_evse_id == evse_id)) {
            tag_pending = false;
            if (reset_pending && !t.transaction_active) {
                log_info("Tag %s seen while a reset is pending. Ignored", pending_tag);
                platform_tag_timed_out21(connection.platform_ctx, evse_id);
            } else if (t.transaction_active) {
                if (strcmp(pending_tag, t.id_token) == 0) {
                    platform_tag_accepted21(connection.platform_ctx, evse_id, pending_tag);
                    stopTransaction(evse_id, TransactionEventTriggerReason::STOP_AUTHORIZED, TransactionEventTransactionInfoStoppedReason::LOCAL, true);
                } else if (authorize_in_flight) {
                    log_info("Tag %s seen while an Authorize is in flight. Ignored", pending_tag);
                } else if (!platform_ws_connected(connection.platform_ctx)) {
                    log_info("Tag %s seen while offline. Only the token of the running transaction can stop it", pending_tag);
                    platform_tag_timed_out21(connection.platform_ctx, evse_id);
                } else {
                    log_info("Tag %s does not match the token of the running transaction. Authorizing it to stop the transaction", pending_tag);
                    authorize_in_flight = true;
                    authorize_for_stop = true;
                    authorize_evse_id = evse_id;
                    strncpy(authorize_token, pending_tag, OCPP21_ID_TOKEN_LEN);
                    authorize_token[OCPP21_ID_TOKEN_LEN] = '\0';

                    AuthorizeIdToken token;
                    token.idToken = authorize_token;
                    token.type = "ISO14443";
                    connection.sendCallAction(Authorize{&token});
                }
            } else if (t.locked_after_stop) {
                if (strcmp(pending_tag, t.id_token) == 0) {
                    log_info("Tag %s unlocks the cable after a local stop", pending_tag);
                    platform_tag_accepted21(connection.platform_ctx, evse_id, pending_tag);
                    platform_unlock_cable21(connection.platform_ctx, evse_id);
                    t.locked_after_stop = false;
                } else {
                    log_info("Tag %s does not match the token of the stopped transaction. Ignored", pending_tag);
                }
            } else if (t.authorized) {
                if (strcmp(pending_tag, t.id_token) == 0) {
                    log_info("Tag %s seen again before the EV was connected. Canceling the authorization", pending_tag);
                    platform_tag_accepted21(connection.platform_ctx, evse_id, pending_tag);
                    t.authorized = false;
                    t.remote_start = false;
                } else {
                    log_info("Tag %s seen while another token is authorized. Ignored", pending_tag);
                }
            } else if (t.tag_window_closed) {
                log_info("Tag %s seen after the tag deadline. Ignored until replug", pending_tag);
            } else if (!authorize_in_flight) {
                if (!platform_ws_connected(connection.platform_ctx)) {
                    // TODO: offline authorization (local auth list or cache)
                    log_info("Tag %s seen while offline. Ignored", pending_tag);
                    platform_tag_timed_out21(connection.platform_ctx, evse_id);
                } else {
                    authorize_in_flight = true;
                    authorize_for_stop = false;
                    authorize_evse_id = evse_id;
                    strncpy(authorize_token, pending_tag, OCPP21_ID_TOKEN_LEN);
                    authorize_token[OCPP21_ID_TOKEN_LEN] = '\0';

                    AuthorizeIdToken token;
                    token.idToken = authorize_token;
                    token.type = "ISO14443";
                    connection.sendCallAction(Authorize{&token});
                }
            }
        }

        // A local stop reported by the platform, e.g. an emergency stop.
        if (stop_pending && stop_evse_id == evse_id) {
            stop_pending = false;
            if (t.transaction_active) {
                t.locked_after_stop = true;
                auto trigger = stop_reason == StopReason21::Remote ? TransactionEventTriggerReason::REMOTE_STOP
                                                                   : TransactionEventTriggerReason::ABNORMAL_CONDITION;
                stopTransaction(evse_id, trigger, stopped_reason_for(stop_reason), false);
            } else {
                log_info("Stop reported on EVSE %d without a transaction. Ignored", evse_id);
            }
        }

        // The EV was not connected in time after the authorization.
        if (t.authorized && !t.transaction_active && !plugged && deadline_elapsed(t.ev_connect_deadline)) {
            log_info("EV connection timeout. Canceling the authorization for %s", t.id_token);
            t.authorized = false;
            t.remote_start = false;
        }

        if (!plugged)
            t.tag_window_closed = false;

        if (s == EvseState21::NotConnected && t.locked_after_stop) {
            t.locked_after_stop = false;
            platform_unlock_cable21(connection.platform_ctx, evse_id);
        }

        bool waiting_for_tag = plugged && !t.transaction_active && !t.authorized && !t.tag_window_closed;
        if (waiting_for_tag && !t.waiting_for_tag) {
            t.tag_deadline = set_deadline((uint32_t)device_model.ev_connection_timeout_s * 1000);
            platform_tag_expected21(connection.platform_ctx, evse_id);
        } else if (!waiting_for_tag && t.waiting_for_tag) {
            platform_clear_tag_expected21(connection.platform_ctx, evse_id);
        }
        t.waiting_for_tag = waiting_for_tag;

        if (t.waiting_for_tag && deadline_elapsed(t.tag_deadline)) {
            log_info("No tag was presented in time on EVSE %d. Replug to start a new transaction", evse_id);
            platform_tag_timed_out21(connection.platform_ctx, evse_id);
            platform_clear_tag_expected21(connection.platform_ctx, evse_id);
            t.tag_window_closed = true;
            t.waiting_for_tag = false;
        }

        // TxStartPoint PowerPathClosed, simplified: authorized and cable plugged.
        if (t.authorized && !t.transaction_active && plugged && !reset_pending)
            startTransaction(evse_id, t.start_trigger);

        if (t.transaction_active) {
            if (s == EvseState21::NotConnected || s == EvseState21::PlugDetected) {
                // TxCtrlr.StopTxOnEVSideDisconnect is fixed to true. A
                // remaining plug (PlugDetected) is an EV side disconnect
                // like in 1.6.
                stopTransaction(evse_id, TransactionEventTriggerReason::EV_COMMUNICATION_LOST, TransactionEventTransactionInfoStoppedReason::EV_DISCONNECTED, false);
            } else if (s != EvseState21::Faulted) {
                auto cs = charging_state_for(s);
                if (cs != t.charging_state) {
                    t.charging_state = cs;
                    sendTransactionUpdated(evse_id, TransactionEventTriggerReason::CHARGING_STATE_CHANGED, false);
                }

                if (device_model.tx_updated_interval_s > 0 && deadline_elapsed(t.next_sampled_value_deadline)) {
                    t.next_sampled_value_deadline = set_deadline((uint32_t)device_model.tx_updated_interval_s * 1000);
                    sendTransactionUpdated(evse_id, TransactionEventTriggerReason::METER_VALUE_PERIODIC, true);
                }
            }
        }

        auto status = connector_status(s, t);
        if (status != t.last_sent_status) {
            t.last_sent_status = status;
            connection.sendCallAction(StatusNotification{
                platform_get_system_time(connection.platform_ctx),
                status,
                evse_id,
                1});
        }

        t.last_state = s;
    }

    // A tag or stop for an unknown EVSE id. Drop it.
    tag_pending = false;
    stop_pending = false;
}

void ChargePoint::startTransaction(int32_t evse_id, TransactionEventTriggerReason trigger)
{
    auto &t = evses[evse_id - 1];

    generate_transaction_id(t.transaction_id);
    t.transaction_active = true;
    t.seq_no = 0;
    t.charging_state = charging_state_for(platform_get_evse_state21(connection.platform_ctx, evse_id));

    log_info("Starting transaction %s on EVSE %d", t.transaction_id, evse_id);

    TransactionEventTransactionInfo info;
    info.transactionId = t.transaction_id;
    info.chargingState = t.charging_state;
    if (t.remote_start)
        info.remoteStartId = t.remote_start_id;

    TransactionEventMeterValueSampledValue sv;
    sv.value = platform_get_energy_wh21(connection.platform_ctx, evse_id);
    sv.measurand = MeterValueSampledValueMeasurand::ENERGY_ACTIVE_IMPORT_REGISTER;
    sv.context = MeterValueSampledValueContext::TRANSACTION_BEGIN;

    TransactionEventMeterValue mv;
    mv.sampledValue = &sv;
    mv.sampledValue_length = 1;
    mv.timestamp = platform_get_system_time(connection.platform_ctx);

    TransactionEventEvse evse;
    evse.id = evse_id;
    evse.connectorId = 1;

    TransactionEventIdToken token;
    token.idToken = t.id_token;
    token.type = t.id_token_type;

    connection.sendTransactionCallAction(TransactionEvent{
        TransactionEventEventType::STARTED,
        platform_get_system_time(connection.platform_ctx),
        trigger,
        t.seq_no++,
        &info,
        nullptr,
        &mv, 1,
        platform_ws_connected(connection.platform_ctx) ? (int8_t)OCPP_BOOL_NOT_PASSED : (int8_t)1,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        TransactionEventPreconditioningStatus::NONE,
        OCPP_BOOL_NOT_PASSED,
        &evse,
        &token});

    platform_lock_cable21(connection.platform_ctx, evse_id);
    platform_set_charging_allowed21(connection.platform_ctx, evse_id, true);
    if (device_model.tx_updated_interval_s > 0)
        t.next_sampled_value_deadline = set_deadline((uint32_t)device_model.tx_updated_interval_s * 1000);
}

void ChargePoint::sendTransactionUpdated(int32_t evse_id, TransactionEventTriggerReason trigger, bool with_meter_value)
{
    auto &t = evses[evse_id - 1];

    TransactionEventTransactionInfo info;
    info.transactionId = t.transaction_id;
    info.chargingState = t.charging_state;

    TransactionEventMeterValueSampledValue sv;
    sv.value = platform_get_energy_wh21(connection.platform_ctx, evse_id);
    sv.measurand = MeterValueSampledValueMeasurand::ENERGY_ACTIVE_IMPORT_REGISTER;
    sv.context = MeterValueSampledValueContext::SAMPLE_PERIODIC;

    TransactionEventMeterValue mv;
    mv.sampledValue = &sv;
    mv.sampledValue_length = 1;
    mv.timestamp = platform_get_system_time(connection.platform_ctx);

    TransactionEventEvse evse;
    evse.id = evse_id;
    evse.connectorId = 1;

    connection.sendTransactionCallAction(TransactionEvent{
        TransactionEventEventType::UPDATED,
        platform_get_system_time(connection.platform_ctx),
        trigger,
        t.seq_no++,
        &info,
        nullptr,
        with_meter_value ? &mv : nullptr, with_meter_value ? (size_t)1 : (size_t)0,
        platform_ws_connected(connection.platform_ctx) ? (int8_t)OCPP_BOOL_NOT_PASSED : (int8_t)1,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        TransactionEventPreconditioningStatus::NONE,
        OCPP_BOOL_NOT_PASSED,
        &evse});
}

void ChargePoint::stopTransaction(int32_t evse_id, TransactionEventTriggerReason trigger, TransactionEventTransactionInfoStoppedReason reason, bool include_token)
{
    auto &t = evses[evse_id - 1];

    platform_set_charging_allowed21(connection.platform_ctx, evse_id, false);

    log_info("Stopping transaction %s on EVSE %d", t.transaction_id, evse_id);

    TransactionEventTransactionInfo info;
    info.transactionId = t.transaction_id;
    info.stoppedReason = reason;
    if (t.remote_start)
        info.remoteStartId = t.remote_start_id;

    TransactionEventMeterValueSampledValue sv;
    sv.value = platform_get_energy_wh21(connection.platform_ctx, evse_id);
    sv.measurand = MeterValueSampledValueMeasurand::ENERGY_ACTIVE_IMPORT_REGISTER;
    sv.context = MeterValueSampledValueContext::TRANSACTION_END;

    TransactionEventMeterValue mv;
    mv.sampledValue = &sv;
    mv.sampledValue_length = 1;
    mv.timestamp = platform_get_system_time(connection.platform_ctx);

    TransactionEventEvse evse;
    evse.id = evse_id;
    evse.connectorId = 1;

    TransactionEventIdToken token;
    token.idToken = t.id_token;
    token.type = t.id_token_type;

    connection.sendTransactionCallAction(TransactionEvent{
        TransactionEventEventType::ENDED,
        platform_get_system_time(connection.platform_ctx),
        trigger,
        t.seq_no++,
        &info,
        nullptr,
        &mv, 1,
        platform_ws_connected(connection.platform_ctx) ? (int8_t)OCPP_BOOL_NOT_PASSED : (int8_t)1,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        OCPP_INTEGER_NOT_PASSED,
        TransactionEventPreconditioningStatus::NONE,
        OCPP_BOOL_NOT_PASSED,
        &evse,
        include_token ? &token : nullptr});

    t.transaction_active = false;
    t.authorized = false;
    t.remote_start = false;
    t.charging_state = TransactionEventTransactionInfoChargingState::NONE;
    t.transaction_id[0] = '\0';
    // 1.6 Finishing parity: a still plugged cable requires a replug before
    // the next tag is accepted. Cleared in tickEvses on unplug.
    t.tag_window_closed = true;
    if (!t.locked_after_stop)
        platform_unlock_cable21(connection.platform_ctx, evse_id);
}

CallResponse ChargePoint::handleBootNotificationResponse(int32_t connectorId, BootNotificationResponseView conf)
{
    (void)connectorId;
    boot_notification_in_flight = false;

    platform_set_system_time(connection.platform_ctx, conf.currentTime());

    int32_t interval = conf.interval();

    switch (conf.status()) {
        case BootNotificationResponseStatus::ACCEPTED:
            log_info("Boot notification accepted");
            state = State::Idle;
            if (interval > 0)
                device_model.heartbeat_interval_s = interval;
            next_heartbeat_deadline = set_deadline((uint32_t)device_model.heartbeat_interval_s * 1000);
            status_notifications_pending = true;
            break;

        case BootNotificationResponseStatus::PENDING:
            log_info("Boot notification pending, retrying in %d s", interval > 0 ? interval : OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S);
            state = State::Pending;
            boot_retry_deadline = set_deadline((uint32_t)(interval > 0 ? interval : OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S) * 1000);
            break;

        case BootNotificationResponseStatus::REJECTED:
            log_warn("Boot notification rejected, retrying in %d s", interval > 0 ? interval : OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S);
            state = State::Rejected;
            boot_retry_deadline = set_deadline((uint32_t)(interval > 0 ? interval : OCPP21_DEFAULT_BOOT_RETRY_INTERVAL_S) * 1000);
            break;
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleHeartbeatResponse(int32_t connectorId, HeartbeatResponseView conf)
{
    (void)connectorId;
    platform_set_system_time(connection.platform_ctx, conf.currentTime());
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleStatusNotificationResponse(int32_t connectorId, StatusNotificationResponseView conf)
{
    (void)connectorId;
    (void)conf;
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleGetVariables(const char *uid, GetVariablesView req)
{
    size_t count = req.getVariableData_count();
    if (count == 0)
        return CallResponse{CallErrorCode::OccurrenceConstraintViolation, "getVariableData must not be empty"};
    if (count > OCPP21_ITEMS_PER_MESSAGE)
        return CallResponse{CallErrorCode::OccurrenceConstraintViolation, "getVariableData exceeds ItemsPerMessage"};

    auto results = heap_alloc_array<GetVariablesResponseGetVariableResult>(count);
    auto components = heap_alloc_array<GetVariablesResponseGetVariableResultComponent>(count);
    auto variables = heap_alloc_array<GetVariablesResponseGetVariableResultVariable>(count);
    auto value_bufs = heap_alloc_array<char[96]>(count);

    for (size_t i = 0; i < count; ++i) {
        auto data = req.getVariableData(i);
        auto component = data.component();
        auto variable = data.variable();

        components[i].name = component.name();
        if (component.instance().is_some())
            components[i].instance = component.instance().unwrap();

        variables[i].name = variable.name();
        if (variable.instance().is_some())
            variables[i].instance = variable.instance().unwrap();

        results[i].component = &components[i];
        results[i].variable = &variables[i];

        if (data.attributeType().is_some())
            results[i].attributeType = (AttributeType)(size_t)data.attributeType().unwrap();

        // Only the Actual attribute is supported.
        if (data.attributeType().is_some() && data.attributeType().unwrap() != AttributeEnumType::ACTUAL) {
            results[i].attributeStatus = GetVariablesResponseGetVariableResultAttributeStatus::NOT_SUPPORTED_ATTRIBUTE_TYPE;
            continue;
        }

        // No EVSE bound components and no component instances exist.
        if (component.evse().is_some() || component.instance().is_some()) {
            results[i].attributeStatus = GetVariablesResponseGetVariableResultAttributeStatus::UNKNOWN_COMPONENT;
            continue;
        }

        auto res = device_model.getVariable(component.name(), variable.name(),
                                            variable.instance().is_some() ? variable.instance().unwrap() : nullptr,
                                            value_bufs[i], sizeof(value_bufs[i]));
        switch (res) {
            case VariableResult::Accepted:
                results[i].attributeStatus = GetVariablesResponseGetVariableResultAttributeStatus::ACCEPTED;
                results[i].attributeValue = value_bufs[i];
                break;
            case VariableResult::UnknownComponent:
                results[i].attributeStatus = GetVariablesResponseGetVariableResultAttributeStatus::UNKNOWN_COMPONENT;
                break;
            case VariableResult::UnknownVariable:
                results[i].attributeStatus = GetVariablesResponseGetVariableResultAttributeStatus::UNKNOWN_VARIABLE;
                break;
            case VariableResult::Rejected:
            case VariableResult::NotSupportedAttributeType:
                results[i].attributeStatus = GetVariablesResponseGetVariableResultAttributeStatus::REJECTED;
                break;
        }
    }

    connection.sendCallResponse(GetVariablesResponse{uid, results.get(), count});
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleSetVariables(const char *uid, SetVariablesView req)
{
    size_t count = req.setVariableData_count();
    if (count == 0)
        return CallResponse{CallErrorCode::OccurrenceConstraintViolation, "setVariableData must not be empty"};
    if (count > OCPP21_ITEMS_PER_MESSAGE)
        return CallResponse{CallErrorCode::OccurrenceConstraintViolation, "setVariableData exceeds ItemsPerMessage"};

    auto results = heap_alloc_array<SetVariablesResponseSetVariableResult>(count);
    auto components = heap_alloc_array<SetVariablesResponseSetVariableResultComponent>(count);
    auto variables = heap_alloc_array<SetVariablesResponseSetVariableResultVariable>(count);

    for (size_t i = 0; i < count; ++i) {
        auto data = req.setVariableData(i);
        auto component = data.component();
        auto variable = data.variable();

        components[i].name = component.name();
        if (component.instance().is_some())
            components[i].instance = component.instance().unwrap();

        variables[i].name = variable.name();
        if (variable.instance().is_some())
            variables[i].instance = variable.instance().unwrap();

        results[i].component = &components[i];
        results[i].variable = &variables[i];

        if (data.attributeType().is_some())
            results[i].attributeType = (AttributeType)(size_t)data.attributeType().unwrap();

        if (data.attributeType().is_some() && data.attributeType().unwrap() != AttributeEnumType::ACTUAL) {
            results[i].attributeStatus = SetVariablesResponseSetVariableResultAttributeStatus::NOT_SUPPORTED_ATTRIBUTE_TYPE;
            continue;
        }

        // No EVSE bound components and no component instances exist.
        if (component.evse().is_some() || component.instance().is_some()) {
            results[i].attributeStatus = SetVariablesResponseSetVariableResultAttributeStatus::UNKNOWN_COMPONENT;
            continue;
        }

        auto res = device_model.setVariable(component.name(), variable.name(),
                                            variable.instance().is_some() ? variable.instance().unwrap() : nullptr,
                                            data.attributeValue());
        switch (res) {
            case VariableResult::Accepted:
                results[i].attributeStatus = SetVariablesResponseSetVariableResultAttributeStatus::ACCEPTED;
                break;
            case VariableResult::UnknownComponent:
                results[i].attributeStatus = SetVariablesResponseSetVariableResultAttributeStatus::UNKNOWN_COMPONENT;
                break;
            case VariableResult::UnknownVariable:
                results[i].attributeStatus = SetVariablesResponseSetVariableResultAttributeStatus::UNKNOWN_VARIABLE;
                break;
            case VariableResult::Rejected:
            case VariableResult::NotSupportedAttributeType:
                results[i].attributeStatus = SetVariablesResponseSetVariableResultAttributeStatus::REJECTED;
                break;
        }
    }

    connection.sendCallResponse(SetVariablesResponse{uid, results.get(), count});

    if (device_model.heartbeat_interval_changed) {
        device_model.heartbeat_interval_changed = false;
        // Apply the new interval to the running heartbeat schedule.
        next_heartbeat_deadline = set_deadline((uint32_t)device_model.heartbeat_interval_s * 1000);
    }

    if (device_model.basic_auth_password_changed || device_model.organization_name_changed || device_model.iso15118_changed) {
        bool password_changed = device_model.basic_auth_password_changed;
        bool iso15118_changed = device_model.iso15118_changed;
        device_model.basic_auth_password_changed = false;
        device_model.organization_name_changed = false;
        device_model.iso15118_changed = false;

        saveSecurityPersistence();

        if (iso15118_changed) {
            platform_cert_store_changed21(connection.platform_ctx);
        }

        if (password_changed) {
            // A01: use the new password from the next connection on. Give
            // the response time to leave before reconnecting. The password
            // content is never logged (A01.FR.12).
            log_info("BasicAuthPassword updated, reconnecting with the new password");
            password_reconnect_deadline = set_deadline(3000);
        }
    }

    if (device_model.network_priority_changed) {
        device_model.network_priority_changed = false;
        saveNetworkPersistence();
        log_info("NetworkConfigurationPriority set to %d, switching after the response left", device_model.network_priority);
        network_reconnect_deadline = set_deadline(3000);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleGetBaseReport(const char *uid, GetBaseReportView req)
{
    // B07.FR.12: ConfigurationInventory and FullInventory are required,
    // SummaryInventory is optional and not supported (B07.FR.02).
    if (req.reportBase() == GetBaseReportReportBase::SUMMARY_INVENTORY) {
        connection.sendCallResponse(GetBaseReportResponse{uid, ReportResponseStatus::NOT_SUPPORTED});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    if (report_active) {
        // B07.FR.13: temporarily unable while another report is streaming.
        connection.sendCallResponse(GetBaseReportResponse{uid, ReportResponseStatus::REJECTED});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    // B07.FR.07: ConfigurationInventory reports the variables that can be
    // set by the operator. B07.FR.08: FullInventory reports everything.
    bool full = req.reportBase() == GetBaseReportReportBase::FULL_INVENTORY;
    uint64_t mask = 0;
    for (size_t i = 0; i < DeviceModel::variableCount(); ++i) {
        if (!device_model.variablePresent(i))
            continue;
        if (full || DeviceModel::variableDesc(i).mutability != VariableMutability::ReadOnly)
            mask |= (uint64_t)1 << i;
    }

    connection.sendCallResponse(GetBaseReportResponse{uid, ReportResponseStatus::ACCEPTED});
    startReport(req.requestId(), mask);
    return CallResponse{CallErrorCode::OK, nullptr};
}

// B08.FR.07/08/09/10/13: criteria are ORed. No component has the Active,
// Available, Enabled or Problem variable, so the first three match every
// component and Problem matches none.
static bool criteria_match(GetReportView &req)
{
    size_t count = req.componentCriteria_count();
    if (count == 0)
        return true;
    for (size_t i = 0; i < count; ++i) {
        auto c = req.componentCriteria(i);
        if (c.is_some() && c.unwrap() != GetReportComponentCriteriaEntry::PROBLEM)
            return true;
    }
    return false;
}

static bool component_variable_match(GetReportView &req, const VariableDesc &desc)
{
    size_t count = req.componentVariable_count();
    if (count == 0)
        return true;

    for (size_t i = 0; i < count; ++i) {
        auto entry_opt = req.componentVariable(i);
        if (entry_opt.is_none())
            continue;
        auto entry = entry_opt.unwrap();
        auto component = entry.component();

        // No EVSE bound components and no component instances exist.
        if (component.evse().is_some() || component.instance().is_some())
            continue;
        if (strcasecmp(component.name(), desc.component) != 0)
            continue;

        // B08.FR.20: a missing variable matches every variable of the component.
        if (entry.variable().is_none())
            return true;
        auto variable = entry.variable().unwrap();
        if (strcasecmp(variable.name(), desc.variable) != 0)
            continue;
        // B08.FR.21: a missing instance matches every instance.
        if (variable.instance().is_none())
            return true;
        if (desc.instance != nullptr && strcasecmp(variable.instance().unwrap(), desc.instance) == 0)
            return true;
    }
    return false;
}

CallResponse ChargePoint::handleGetReport(const char *uid, GetReportView req)
{
    if (report_active) {
        // B08.FR.16: temporarily unable while another report is streaming.
        connection.sendCallResponse(GetReportResponse{uid, ReportResponseStatus::REJECTED});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    // B08.FR.17: more componentVariable entries than ItemsPerMessage.
    if (req.componentVariable_count() > OCPP21_ITEMS_PER_MESSAGE)
        return CallResponse{CallErrorCode::OccurrenceConstraintViolation, "componentVariable exceeds ItemsPerMessage"};

    uint64_t mask = 0;
    if (criteria_match(req)) {
        for (size_t i = 0; i < DeviceModel::variableCount(); ++i) {
            if (device_model.variablePresent(i) && component_variable_match(req, DeviceModel::variableDesc(i)))
                mask |= (uint64_t)1 << i;
        }
    }

    if (mask == 0) {
        // B08.FR.15, no NotifyReport follows (B08.FR.05).
        connection.sendCallResponse(GetReportResponse{uid, ReportResponseStatus::EMPTY_RESULT_SET});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    connection.sendCallResponse(GetReportResponse{uid, ReportResponseStatus::ACCEPTED});
    startReport(req.requestId(), mask);
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleNotifyReportResponse(int32_t connectorId, NotifyReportResponseView conf)
{
    (void)connectorId;
    (void)conf;
    // The next chunk is sent from the next tick.
    report_in_flight = false;
    return CallResponse{CallErrorCode::OK, nullptr};
}

void ChargePoint::startReport(int32_t request_id, uint64_t mask)
{
    report_active = true;
    report_in_flight = false;
    report_request_id = request_id;
    report_seq_no = 0;
    report_next_idx = 0;
    report_mask = mask;
}

void ChargePoint::abortReport()
{
    report_active = false;
    report_in_flight = false;
}

#define OCPP21_REPORT_CHUNK_SIZE 8

void ChargePoint::sendReportChunk()
{
    NotifyReportReportData report_data[OCPP21_REPORT_CHUNK_SIZE];
    NotifyReportReportDataComponent components[OCPP21_REPORT_CHUNK_SIZE];
    NotifyReportReportDataVariable variables[OCPP21_REPORT_CHUNK_SIZE];
    NotifyReportReportDataVariableAttribute attributes[OCPP21_REPORT_CHUNK_SIZE];
    NotifyReportReportDataVariableCharacteristics characteristics[OCPP21_REPORT_CHUNK_SIZE];
    char value_bufs[OCPP21_REPORT_CHUNK_SIZE][96];

    size_t n = 0;
    size_t idx = report_next_idx;
    for (; idx < DeviceModel::variableCount() && n < OCPP21_REPORT_CHUNK_SIZE; ++idx) {
        if ((report_mask & ((uint64_t)1 << idx)) == 0)
            continue;

        const auto &desc = DeviceModel::variableDesc(idx);

        components[n].name = desc.component;
        variables[n].name = desc.variable;
        variables[n].instance = desc.instance;

        // B07.FR.11: all supported attribute types, only Actual exists.
        attributes[n].type = AttributeType::ACTUAL;
        attributes[n].mutability = (NotifyReportReportDataVariableAttributeMutability)(size_t)desc.mutability;
        attributes[n].persistent = desc.persistent ? 1 : 0;
        attributes[n].constant = desc.constant ? 1 : 0;
        // B07.FR.03: the value of WriteOnly variables is omitted.
        if (desc.mutability != VariableMutability::WriteOnly
         && device_model.getVariableByIndex(idx, value_bufs[n], sizeof(value_bufs[n])) == VariableResult::Accepted)
            attributes[n].value = value_bufs[n];

        characteristics[n].dataType = (NotifyReportReportDataVariableCharacteristicsDataType)(size_t)desc.data_type;
        characteristics[n].supportsMonitoring = false;
        if (desc.max_limit >= 0)
            characteristics[n].maxLimit = desc.max_limit;
        characteristics[n].valuesList = desc.values_list;
        characteristics[n].unit = desc.unit;

        report_data[n].component = &components[n];
        report_data[n].variable = &variables[n];
        report_data[n].variableAttribute = &attributes[n];
        report_data[n].variableAttribute_length = 1;
        report_data[n].variableCharacteristics = &characteristics[n];
        ++n;
    }

    bool more = (report_mask >> idx) != 0 && idx < DeviceModel::variableCount();

    if (!connection.sendCallAction(NotifyReport{report_request_id,
                                                platform_get_system_time(connection.platform_ctx),
                                                report_seq_no,
                                                report_data, n,
                                                (int8_t)(more ? 1 : 0)})) {
        abortReport();
        return;
    }

    report_in_flight = true;
    report_next_idx = idx;
    ++report_seq_no;

    if (!more)
        report_active = false;
}

CallResponse ChargePoint::handleTriggerMessage(const char *uid, TriggerMessageView req)
{
    auto status = TriggerMessageResponseStatus::NOT_IMPLEMENTED;

    switch (req.requestedMessage()) {
        case TriggerMessageRequestedMessage::BOOT_NOTIFICATION:
            // B06: a triggered boot notification is only useful while not accepted.
            if (state == State::Idle) {
                status = TriggerMessageResponseStatus::REJECTED;
            } else {
                trigger_boot_notification = true;
                status = TriggerMessageResponseStatus::ACCEPTED;
            }
            break;

        case TriggerMessageRequestedMessage::HEARTBEAT:
            trigger_heartbeat = true;
            status = TriggerMessageResponseStatus::ACCEPTED;
            break;

        case TriggerMessageRequestedMessage::STATUS_NOTIFICATION:
            trigger_status_notification = true;
            status = TriggerMessageResponseStatus::ACCEPTED;
            break;

        // A02: the CSR is generated and sent from the next tick so this
        // response leaves first.
        case TriggerMessageRequestedMessage::SIGN_CHARGING_STATION_CERTIFICATE:
            trigger_sign = true;
            trigger_sign_type = SignCertificateCertificateType::CHARGING_STATION_CERTIFICATE;
            status = TriggerMessageResponseStatus::ACCEPTED;
            break;

        case TriggerMessageRequestedMessage::SIGN_V2_G_CERTIFICATE:
            if (!device_model.v2g_cert_install_enabled) {
                status = TriggerMessageResponseStatus::REJECTED;
                break;
            }
            trigger_sign = true;
            trigger_sign_type = SignCertificateCertificateType::V2_G_CERTIFICATE;
            status = TriggerMessageResponseStatus::ACCEPTED;
            break;

        case TriggerMessageRequestedMessage::SIGN_V2_G20_CERTIFICATE:
            if (!device_model.v2g_cert_install_enabled) {
                status = TriggerMessageResponseStatus::REJECTED;
                break;
            }
            trigger_sign = true;
            trigger_sign_type = SignCertificateCertificateType::V2_G20_CERTIFICATE;
            status = TriggerMessageResponseStatus::ACCEPTED;
            break;

        // One certificate for both the CSMS connection and ISO 15118,
        // certificateType omitted on the wire (NONE).
        case TriggerMessageRequestedMessage::SIGN_COMBINED_CERTIFICATE:
            if (!device_model.v2g_cert_install_enabled) {
                status = TriggerMessageResponseStatus::REJECTED;
                break;
            }
            trigger_sign = true;
            trigger_sign_type = SignCertificateCertificateType::NONE;
            status = TriggerMessageResponseStatus::ACCEPTED;
            break;

        default:
            status = TriggerMessageResponseStatus::NOT_IMPLEMENTED;
            break;
    }

    connection.sendCallResponse(TriggerMessageResponse{uid, status});
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleReset(const char *uid, ResetView req)
{
    // Resuming a transaction after the reboot needs a persistent
    // transaction queue, not implemented yet.
    if (req.type() == ResetType::IMMEDIATE_AND_RESUME) {
        connection.sendCallResponse(ResetResponse{uid, ResetResponseStatus::REJECTED});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    if (req.evseId().is_some() && (req.evseId().unwrap() < 1 || req.evseId().unwrap() > OCPP21_NUM_EVSES)) {
        connection.sendCallResponse(ResetResponse{uid, ResetResponseStatus::REJECTED});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    bool transaction_running = false;
    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id)
        transaction_running |= evses[evse_id - 1].transaction_active;

    if (req.type() == ResetType::ON_IDLE) {
        // B12.FR.02: finish the running transaction first. No new
        // transactions start while the reset is pending.
        log_info("Reset accepted, waiting until %s", transaction_running ? "the transaction ended" : "the event queue is empty");
        connection.sendCallResponse(ResetResponse{uid, transaction_running ? ResetResponseStatus::SCHEDULED : ResetResponseStatus::ACCEPTED});
        reset_pending = true;
        reset_on_idle = true;
        reset_drain_deadline = 0;
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    connection.sendCallResponse(ResetResponse{uid, ResetResponseStatus::ACCEPTED});

    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
        if (evses[evse_id - 1].transaction_active)
            stopTransaction(evse_id, TransactionEventTriggerReason::RESET_COMMAND, TransactionEventTransactionInfoStoppedReason::IMMEDIATE_RESET, false);
    }

    // The reset happens from tickReset once the response and the queued
    // transaction events had time to leave.
    reset_pending = true;
    reset_on_idle = false;
    reset_drain_deadline = 0;
    return CallResponse{CallErrorCode::OK, nullptr};
}

// Drain timeout for the queued transaction events before a reset. Without
// a persistent queue a reset while offline would otherwise never happen.
#define OCPP21_RESET_DRAIN_TIMEOUT_MS (10 * 1000)

void ChargePoint::tickReset()
{
    bool transaction_running = false;
    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id)
        transaction_running |= evses[evse_id - 1].transaction_active;

    if (reset_on_idle && transaction_running)
        return;

    if (reset_drain_deadline == 0)
        reset_drain_deadline = set_deadline(OCPP21_RESET_DRAIN_TIMEOUT_MS);

    bool queue_empty = connection.transaction_messages.empty() && !connection.in_flight_is_transaction;
    if (!queue_empty && !deadline_elapsed(reset_drain_deadline))
        return;

    log_info("Resetting");
    reset_pending = false;
    reset_drain_deadline = 0;
    platform_reset(false);
}

CallResponse ChargePoint::handleAuthorizeResponse(int32_t connectorId, AuthorizeResponseView conf)
{
    (void)connectorId;
    authorize_in_flight = false;
    bool for_stop = authorize_for_stop;
    authorize_for_stop = false;

    if (authorize_evse_id < 1 || authorize_evse_id > OCPP21_NUM_EVSES)
        return CallResponse{CallErrorCode::OK, nullptr};

    auto &t = evses[authorize_evse_id - 1];
    auto status = conf.idTokenInfo().status();

    if (status != ResponseIdTokenInfoEntriesStatus::ACCEPTED) {
        log_info("Authorization of %s rejected (%s)", authorize_token, ResponseIdTokenInfoEntriesStatusStrings[(size_t)status]);
        platform_tag_rejected21(connection.platform_ctx, authorize_evse_id, authorize_token, rejection_type(status));
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    if (for_stop) {
        if (!t.transaction_active) {
            log_info("Authorization of %s accepted, but the transaction already ended. Ignored", authorize_token);
            return CallResponse{CallErrorCode::OK, nullptr};
        }
        log_info("Authorization of %s accepted, stopping the transaction", authorize_token);
        platform_tag_accepted21(connection.platform_ctx, authorize_evse_id, authorize_token);
        // The Ended event reports the token that stopped the transaction.
        memcpy(t.id_token, authorize_token, sizeof(t.id_token));
        strncpy(t.id_token_type, "ISO14443", OCPP21_ID_TOKEN_TYPE_LEN);
        t.id_token_type[OCPP21_ID_TOKEN_TYPE_LEN] = '\0';
        stopTransaction(authorize_evse_id, TransactionEventTriggerReason::STOP_AUTHORIZED, TransactionEventTransactionInfoStoppedReason::LOCAL, true);
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    if (t.transaction_active || t.authorized) {
        log_info("Authorization of %s accepted, but the EVSE is no longer free. Ignored", authorize_token);
        platform_tag_rejected21(connection.platform_ctx, authorize_evse_id, authorize_token, TagRejectionType21::ConcurrentTx);
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    log_info("Authorization of %s accepted", authorize_token);
    platform_tag_accepted21(connection.platform_ctx, authorize_evse_id, authorize_token);

    memcpy(t.id_token, authorize_token, sizeof(t.id_token));
    strncpy(t.id_token_type, "ISO14443", OCPP21_ID_TOKEN_TYPE_LEN);
    t.id_token_type[OCPP21_ID_TOKEN_TYPE_LEN] = '\0';
    t.authorized = true;
    t.remote_start = false;

    auto s = platform_get_evse_state21(connection.platform_ctx, authorize_evse_id);
    bool plugged = evse_plugged(s);
    // If the cable is already plugged the authorization completes the start
    // condition. Otherwise the plug event will.
    t.start_trigger = plugged ? TransactionEventTriggerReason::AUTHORIZED
                              : TransactionEventTriggerReason::CABLE_PLUGGED_IN;
    t.ev_connect_deadline = set_deadline((uint32_t)device_model.ev_connection_timeout_s * 1000);

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleTransactionEventResponse(int32_t connectorId, TransactionEventResponseView conf)
{
    (void)connectorId;

    // The response does not repeat the transaction id. As long as only one
    // EVSE exists the running transaction is unambiguous.
    auto idTokenInfo = conf.idTokenInfo();
    if (idTokenInfo.is_some() && idTokenInfo.unwrap().status() != ResponseIdTokenInfoEntriesStatus::ACCEPTED) {
        for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
            auto &t = evses[evse_id - 1];
            if (!t.transaction_active)
                continue;
            log_info("Transaction %s deauthorized by the CSMS", t.transaction_id);
            stopTransaction(evse_id, TransactionEventTriggerReason::DEAUTHORIZED, TransactionEventTransactionInfoStoppedReason::DE_AUTHORIZED, false);
        }
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleMeterValuesResponse(int32_t connectorId, MeterValuesResponseView conf)
{
    (void)connectorId;
    (void)conf;
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleSecurityEventNotificationResponse(int32_t connectorId, SecurityEventNotificationResponseView conf)
{
    (void)connectorId;
    (void)conf;
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleRequestStartTransaction(const char *uid, RequestStartTransactionView req)
{
    int32_t evse_id = req.evseId().is_some() ? req.evseId().unwrap() : 1;

    if (evse_id < 1 || evse_id > OCPP21_NUM_EVSES) {
        connection.sendCallResponse(RequestStartTransactionResponse{uid, ResponseStatus::REJECTED});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    auto &t = evses[evse_id - 1];
    if (t.transaction_active || t.authorized || authorize_in_flight || reset_pending) {
        connection.sendCallResponse(RequestStartTransactionResponse{uid, ResponseStatus::REJECTED});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    auto idToken = req.idToken();
    strncpy(t.id_token, idToken.idToken(), OCPP21_ID_TOKEN_LEN);
    t.id_token[OCPP21_ID_TOKEN_LEN] = '\0';
    strncpy(t.id_token_type, idToken.type(), OCPP21_ID_TOKEN_TYPE_LEN);
    t.id_token_type[OCPP21_ID_TOKEN_TYPE_LEN] = '\0';

    // AuthCtrlr.AuthorizeRemoteStart is fixed to false: the token counts as
    // authorized without an Authorize round trip.
    t.authorized = true;
    t.remote_start = true;
    t.remote_start_id = req.remoteStartId();
    t.start_trigger = TransactionEventTriggerReason::REMOTE_START;
    t.ev_connect_deadline = set_deadline((uint32_t)device_model.ev_connection_timeout_s * 1000);

    if (req.chargingProfile().is_some())
        log_info("RequestStartTransaction contains a charging profile. Ignored, smart charging is not implemented yet");

    log_info("Remote start accepted for token %s on EVSE %d", t.id_token, evse_id);
    connection.sendCallResponse(RequestStartTransactionResponse{uid, ResponseStatus::ACCEPTED});
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleRequestStopTransaction(const char *uid, RequestStopTransactionView req)
{
    for (int32_t evse_id = 1; evse_id <= OCPP21_NUM_EVSES; ++evse_id) {
        auto &t = evses[evse_id - 1];
        if (!t.transaction_active || strcmp(t.transaction_id, req.transactionId()) != 0)
            continue;

        log_info("Remote stop accepted for transaction %s", t.transaction_id);
        connection.sendCallResponse(RequestStopTransactionResponse{uid, ResponseStatus::ACCEPTED});
        stopTransaction(evse_id, TransactionEventTriggerReason::REMOTE_STOP, TransactionEventTransactionInfoStoppedReason::REMOTE, false);
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    log_info("Remote stop rejected, unknown transaction %s", req.transactionId());
    connection.sendCallResponse(RequestStopTransactionResponse{uid, ResponseStatus::REJECTED});
    return CallResponse{CallErrorCode::OK, nullptr};
}

// Certificate management (A02/A03, M03/M04/M05, M06/M07)

#define OCPP21_A03_RENEWAL_WINDOW_S (30 * 24 * 3600)
#define OCPP21_OCSP_MAX_CACHE_S (7 * 24 * 3600)
#define OCPP21_OCSP_RETRY_MS (3600 * 1000)

// Indexed by SignCertificateCertificateType, NONE is the combined certificate.
static const char * const sign_type_names[] = {"ChargingStationCertificate", "V2GCertificate", "V2G20Certificate", "CombinedCertificate"};

static CertGroup chain_group_for_sign_type(SignCertificateCertificateType type)
{
    switch (type) {
        case SignCertificateCertificateType::CHARGING_STATION_CERTIFICATE: return CertGroup::CsmsClientChain;
        case SignCertificateCertificateType::V2_G_CERTIFICATE:             return CertGroup::V2GChain;
        case SignCertificateCertificateType::V2_G20_CERTIFICATE:           return CertGroup::V2G20Chain;
        default:                                                           return CertGroup::None;
    }
}

void ChargePoint::tickCertificates()
{
    if (trigger_sign) {
        trigger_sign = false;
        startCsr(trigger_sign_type, false, nullptr);
    }

    // A02.FR.17/18/19: resend the same CSR with doubling backoff until
    // CertSigningRepeatTimes is exhausted.
    if (csr_active && deadline_elapsed(csr_retry_deadline)) {
        if (csr_attempts_left > 0) {
            --csr_attempts_left;
            csr_backoff_s *= 2;
            log_info("No CertificateSigned received, resending the CSR (requestId %d)", csr_request_id);
            sendSignCertificate();
        } else {
            log_warn("CSR retries exhausted, resuming only on TriggerMessage");
            abortCsr();
        }
    }

    // A03.FR.02: renew certificates expiring within one month.
    if (deadline_elapsed(cert_expiry_check_deadline)) {
        cert_expiry_check_deadline = set_deadline(6 * 3600 * 1000);
        time_t now = platform_get_system_time(connection.platform_ctx);
        if (!csr_active) {
            for (auto &e : cert_store.all()) {
                SignCertificateCertificateType type;
                switch (e.group) {
                    case CertGroup::CsmsClientChain: type = SignCertificateCertificateType::CHARGING_STATION_CERTIFICATE; break;
                    case CertGroup::V2GChain:        type = SignCertificateCertificateType::V2_G_CERTIFICATE; break;
                    case CertGroup::V2G20Chain:      type = SignCertificateCertificateType::V2_G20_CERTIFICATE; break;
                    default: continue;
                }
                if (type != SignCertificateCertificateType::CHARGING_STATION_CERTIFICATE
                 && !device_model.v2g_cert_install_enabled) {
                    // A03 for the V2G types is turned off (V2GCertificateInstallationEnabled).
                    continue;
                }
                if (e.not_after == 0 || e.not_after - now > OCPP21_A03_RENEWAL_WINDOW_S) {
                    continue;
                }
                // A chain installed under both groups is a combined
                // certificate and renews as one (certificateType omitted).
                if ((e.group == CertGroup::CsmsClientChain && cert_store.findSeccChainById(e.id) != nullptr)
                 || (e.group == CertGroup::V2GChain && cert_store.find(CertGroup::CsmsClientChain) != nullptr
                     && cert_store.find(CertGroup::CsmsClientChain)->id == e.id)) {
                    type = SignCertificateCertificateType::NONE;
                }
                log_info("The %s expires soon, requesting renewal (A03)", sign_type_names[(size_t)type]);
                startCsr(type, true, e.has_anchor ? &e.anchor_root : nullptr);
                break;
            }
        }
    }

    // M06: refresh cached OCSP status, one request in flight at a time.
    if (ocsp_in_flight_idx < 0) {
        for (int32_t i = 0; i < (int32_t)OCPP21_OCSP_CACHE_SIZE; ++i) {
            auto &slot = ocsp_cache[i];
            if (!slot.used || !deadline_elapsed(slot.refresh_deadline)) {
                continue;
            }
            GetCertificateStatusOcspRequestData data;
            data.hashAlgorithm = HashAlgorithm::SHA256;
            data.issuerNameHash = slot.hash.issuer_name_hash;
            data.issuerKeyHash = slot.hash.issuer_key_hash;
            data.serialNumber = slot.hash.serial_number;
            data.responderURL = slot.url;
            if (connection.sendCallAction(GetCertificateStatus{&data})) {
                slot.in_flight = true;
                ocsp_in_flight_idx = i;
            }
            break;
        }
    }
}

void ChargePoint::startCsr(SignCertificateCertificateType type, bool renewal, const OcppCertHashData21 *root_hash)
{
    // HUB20-42-003: a new trigger aborts a pending retry.
    abortCsr();

    csr_pending_id = cert_store.nextId();
    std::string key_path = cert_store.keyPath(csr_pending_id);

    OcppCsrParams21 params;
    params.key_name = key_path.c_str();
    switch (type) {
        case SignCertificateCertificateType::CHARGING_STATION_CERTIFICATE:
            params.curve = OcppCurve21::Secp256r1;
            params.common_name = charge_point_name.c_str();
            params.organization = device_model.organization_name[0] != '\0' ? device_model.organization_name : nullptr;
            params.country = nullptr;
            params.domain_component = nullptr;
            break;
        case SignCertificateCertificateType::V2_G_CERTIFICATE:
            // ISO 15118-2 SECC leaf (A02.FR.22). The certificate profile
            // requires domainComponent "CPO", the EVCC verifies it and
            // aborts the TLS setup without it (V2G2-875).
            params.curve = OcppCurve21::Secp256r1;
            params.common_name = device_model.secc_id;
            params.organization = device_model.iso_organization_name;
            params.country = device_model.country_name;
            params.domain_component = "CPO";
            break;
        case SignCertificateCertificateType::V2_G20_CERTIFICATE:
            // ISO 15118-20 SECC leaf, crypto from V2G20SECCLeafCryptoSuite
            // (A02.FR.23). The -20 certificate profile requires a
            // domainComponent ending in "CSO".
            params.curve = device_model.v2g20_use_ed448 ? OcppCurve21::Ed448 : OcppCurve21::Secp521r1;
            params.common_name = device_model.secc_id;
            params.organization = device_model.iso_organization_name;
            params.country = device_model.country_name;
            params.domain_component = "CSO";
            break;
        case SignCertificateCertificateType::NONE:
            // Combined certificate, used for the CSMS connection and as the
            // ISO 15118-2 SECC leaf. The V2G PKI dictates the subject.
            params.curve = OcppCurve21::Secp256r1;
            params.common_name = device_model.secc_id;
            params.organization = device_model.iso_organization_name;
            params.country = device_model.country_name;
            params.domain_component = "CPO";
            break;
        default:
            return;
    }

    if (platform_generate_csr21(&params, csr_buf, sizeof(csr_buf)) == 0) {
        log_error("Failed to generate a key pair and CSR for the %s", sign_type_names[(size_t)type]);
        csr_pending_id = 0;
        return;
    }

    csr_active = true;
    csr_type = type;
    csr_request_id = ++last_sign_request_id;
    csr_attempts_left = device_model.cert_signing_repeat_times;
    csr_backoff_s = (uint32_t)device_model.cert_signing_wait_minimum_s;
    // A03.FR.23: the renewal CSR identifies the issuing PKI. Never sent
    // for A02 (HUB20-421-002).
    csr_has_root_hash = renewal && root_hash != nullptr;
    if (csr_has_root_hash) {
        csr_root_hash = *root_hash;
    }

    log_info("Sending CSR for the %s (requestId %d)", sign_type_names[(size_t)type], csr_request_id);
    sendSignCertificate();
}

void ChargePoint::abortCsr()
{
    if (csr_pending_id != 0) {
        platform_remove_file(cert_store.keyPath(csr_pending_id).c_str());
    }
    csr_pending_id = 0;
    csr_active = false;
}

void ChargePoint::sendSignCertificate()
{
    SignCertificateHashRootCertificate root;
    if (csr_has_root_hash) {
        root.hashAlgorithm = HashAlgorithm::SHA256;
        root.issuerNameHash = csr_root_hash.issuer_name_hash;
        root.issuerKeyHash = csr_root_hash.issuer_key_hash;
        root.serialNumber = csr_root_hash.serial_number;
    }

    connection.sendCallAction(SignCertificate{csr_buf, csr_type, csr_has_root_hash ? &root : nullptr, csr_request_id});
    csr_retry_deadline = set_deadline(csr_backoff_s * 1000);
}

CallResponse ChargePoint::handleSignCertificateResponse(int32_t connectorId, SignCertificateResponseView conf)
{
    (void)connectorId;
    if (!csr_active) {
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    if (conf.status() == SignCertificateResponseStatus::REJECTED) {
        // A02.FR.20: no backoff resend until the next TriggerMessage.
        // HUB20-421-001: a rejected V2G CSR does not turn off ISO 15118.
        log_warn("SignCertificate rejected by the CSMS, resuming only on TriggerMessage");
        abortCsr();
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleCertificateSigned(const char *uid, CertificateSignedView req)
{
    const char *chain = req.certificateChain();
    const char *reject_reason = nullptr;
    size_t cert_count = 0;
    bool combined = csr_type == SignCertificateCertificateType::NONE;
    CertGroup chain_group = chain_group_for_sign_type(csr_type);
    bool needs_csms_roots = chain_group == CertGroup::CsmsClientChain || combined;
    bool needs_v2g_roots = chain_group != CertGroup::CsmsClientChain || combined;

    // The combined certificate verifies against the CSMS and V2G roots.
    std::unique_ptr<char[]> root_bufs[OCPP21_CERTSTORE_MAX_V2G_ROOT + OCPP21_CERTSTORE_MAX_CSMS_ROOT];
    const char *root_ptrs[OCPP21_CERTSTORE_MAX_V2G_ROOT + OCPP21_CERTSTORE_MAX_CSMS_ROOT];
    size_t roots = 0;
    size_t anchor_idx = 0;

    if (!csr_active) {
        reject_reason = "NoPendingCsr";
    } else if (req.requestId().is_some() && req.requestId().unwrap() != csr_request_id) {
        // A02.FR.26: unknown requestId.
        reject_reason = "UnknownRequestId";
    } else if (req.certificateType().is_some() && (size_t)req.certificateType().unwrap() != (size_t)csr_type) {
        reject_reason = "TypeMismatch";
    } else if ((cert_count = platform_cert_count21(chain)) == 0 || cert_count > OCPP21_CHAIN_MAX_CHILDREN + 1) {
        reject_reason = "InvalidChain";
    } else if (!platform_key_matches_cert21(cert_store.keyPath(csr_pending_id).c_str(), chain)) {
        reject_reason = "KeyMismatch";
    } else {
        // HUB20-42-004: the chain must not include the root.
        for (size_t i = 0; i < cert_count; ++i) {
            OcppCertInfo21 info;
            if (!platform_cert_info21(chain, i, &info)) {
                reject_reason = "InvalidChain";
                break;
            }
            if (info.self_signed) {
                reject_reason = "ChainIncludesRoot";
                break;
            }
        }
    }

    if (reject_reason == nullptr) {
        // HUB20-42-006: validate against the installed roots.
        if (needs_csms_roots) {
            roots += cert_store.loadRoots(CertGroup::CsmsRoot, root_bufs, root_ptrs, OCPP21_CERTSTORE_MAX_CSMS_ROOT);
        }
        if (needs_v2g_roots) {
            roots += cert_store.loadRoots(CertGroup::V2GRoot, root_bufs + roots, root_ptrs + roots, OCPP21_CERTSTORE_MAX_V2G_ROOT);
        }
        if (roots == 0) {
            reject_reason = "NoTrustedRoot";
        } else {
            time_t now = platform_get_system_time(connection.platform_ctx);
            auto result = platform_verify_chain21(chain, root_ptrs, roots, now, &anchor_idx);
            if (result == OcppChainVerifyResult21::NotYetValid) {
                // HUB20-42-001: accept a validity start up to 300 s in the future.
                OcppCertInfo21 leaf;
                if (platform_cert_info21(chain, 0, &leaf) && leaf.not_before <= now + 300) {
                    result = platform_verify_chain21(chain, root_ptrs, roots, leaf.not_before + 1, &anchor_idx);
                }
            }
            if (result != OcppChainVerifyResult21::Ok) {
                reject_reason = result == OcppChainVerifyResult21::Untrusted ? "UntrustedChain" : "InvalidChain";
            }
        }
    }

    if (reject_reason != nullptr) {
        log_warn("CertificateSigned rejected: %s", reject_reason);
        CertificateSignedResponseStatusInfo info;
        info.reasonCode = reject_reason;
        connection.sendCallResponse(CertificateSignedResponse{uid, ResponseStatus::REJECTED, &info});
        // A02.FR.07: security event only for the charging station
        // certificate, not for the V2G types.
        if (csr_active && (combined || csr_type == SignCertificateCertificateType::CHARGING_STATION_CERTIFICATE)) {
            sendSecurityEventNotification("InvalidChargingStationCertificate", reject_reason);
        }
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    // The combined certificate serves both connections, the V2G copy is
    // installed first so that findSeccChainById and the boot scan see it.
    OcppCertHashData21 anchor_hash;
    bool installed = platform_cert_hash_data21(root_ptrs[anchor_idx], 0, nullptr, 0, &anchor_hash);
    if (installed && combined) {
        installed = cert_store.installChain(CertGroup::V2GChain, csr_pending_id, chain, anchor_hash)
                 && cert_store.installChain(CertGroup::CsmsClientChain, csr_pending_id, chain, anchor_hash);
    } else if (installed) {
        installed = cert_store.installChain(chain_group, csr_pending_id, chain, anchor_hash);
    }
    if (!installed) {
        log_error("Failed to store the signed certificate chain");
        CertificateSignedResponseStatusInfo info;
        info.reasonCode = "StorageFailure";
        connection.sendCallResponse(CertificateSignedResponse{uid, ResponseStatus::REJECTED, &info});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    connection.sendCallResponse(CertificateSignedResponse{uid, ResponseStatus::ACCEPTED});
    log_info("Installed the signed %s", sign_type_names[(size_t)csr_type]);

    uint32_t chain_id = csr_pending_id;
    csr_pending_id = 0; // the key now belongs to the installed chain
    csr_active = false;
    platform_cert_store_changed21(connection.platform_ctx);

    if (combined || chain_group == CertGroup::CsmsClientChain) {
        // A02.FR.08: reconnect with the new certificate after the
        // response left. No reconnect for the V2G types.
        if (tls_in_use) {
            pending_client_chain_id = chain_id;
            cert_reconnect_deadline = set_deadline(3000);
        }
    }
    if (combined || chain_group != CertGroup::CsmsClientChain) {
        // M06.FR.07: refresh the OCSP status of the new chain.
        scheduleChainOcsp(chain_id);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

void ChargePoint::applyClientCertificate(uint32_t chain_id)
{
    tls_client_cert_file = cert_store.pemPath(CertGroup::CsmsClientChain, chain_id);
    tls_client_key_file = cert_store.keyPath(chain_id);

    PlatformTlsConfig tls;
    tls.ca_cert_file = tls_ca_file.empty() ? nullptr : tls_ca_file.c_str();
    tls.client_cert_file = tls_client_cert_file.c_str();
    tls.client_key_file = tls_client_key_file.c_str();
    platform_update_tls(connection.platform_ctx, &tls);

    log_info("Reconnecting with the new charging station certificate");
    platform_reconnect(connection.platform_ctx);
}

CallResponse ChargePoint::handleInstallCertificate(const char *uid, InstallCertificateView req)
{
    // HUB20-21-005: trust-store changes require an authenticated,
    // integrity-protected OCPP connection (Security Profile 2 or 3).
    if ((device_model.security_profile < 2) || !tls_in_use) {
        log_warn("Refused to install a certificate over Security Profile %d", device_model.security_profile);
        connection.sendCallResponse(InstallCertificateResponse{uid, eResponseStatus::REJECTED});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    CertGroup group;
    switch (req.certificateType()) {
        case InstallCertificateCertificateType::V2_G_ROOT_CERTIFICATE:          group = CertGroup::V2GRoot; break;
        case InstallCertificateCertificateType::MO_ROOT_CERTIFICATE:            group = CertGroup::MORoot; break;
        case InstallCertificateCertificateType::MANUFACTURER_ROOT_CERTIFICATE:  group = CertGroup::MfrRoot; break;
        case InstallCertificateCertificateType::CSMS_ROOT_CERTIFICATE:          group = CertGroup::CsmsRoot; break;
        case InstallCertificateCertificateType::OEM_ROOT_CERTIFICATE:           group = CertGroup::OEMRoot; break;
        default:
            return CallResponse{CallErrorCode::PropertyConstraintViolation, "unknown certificateType"};
    }

    auto status = eResponseStatus::REJECTED;
    switch (cert_store.installRoot(group, req.certificate(), platform_get_system_time(connection.platform_ctx))) {
        case CertInstallResult::Accepted:
            status = eResponseStatus::ACCEPTED;
            log_info("Installed a %s", InstallCertificateCertificateTypeStrings[(size_t)req.certificateType()]);
            platform_cert_store_changed21(connection.platform_ctx);
            break;
        case CertInstallResult::Rejected:
            status = eResponseStatus::REJECTED;
            log_warn("Rejected a %s", InstallCertificateCertificateTypeStrings[(size_t)req.certificateType()]);
            break;
        case CertInstallResult::Failed:
            status = eResponseStatus::FAILED;
            log_error("Failed to store a %s", InstallCertificateCertificateTypeStrings[(size_t)req.certificateType()]);
            break;
    }

    connection.sendCallResponse(InstallCertificateResponse{uid, status});
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleDeleteCertificate(const char *uid, DeleteCertificateView req)
{
    if ((device_model.security_profile < 2) || !tls_in_use) {
        log_warn("Refused to delete a certificate over Security Profile %d", device_model.security_profile);
        connection.sendCallResponse(DeleteCertificateResponse{uid, DeleteCertificateResponseStatus::FAILED});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    auto hash_data = req.certificateHashData();
    auto status = DeleteCertificateResponseStatus::NOT_FOUND;

    // Only SHA256 hashes are stored (A00.FR.506).
    if (hash_data.hashAlgorithm() == CertificateHashDataEntriesHashAlgorithm::SHA256) {
        switch (cert_store.deleteByHash(hash_data.issuerNameHash(), hash_data.issuerKeyHash(), hash_data.serialNumber())) {
            case CertDeleteResult::Accepted:
                status = DeleteCertificateResponseStatus::ACCEPTED;
                log_info("Deleted the certificate with serial %s", hash_data.serialNumber());
                platform_cert_store_changed21(connection.platform_ctx);
                break;
            case CertDeleteResult::Failed:
                status = DeleteCertificateResponseStatus::FAILED;
                log_warn("Refused to delete the certificate in use for the CSMS connection");
                break;
            case CertDeleteResult::NotFound:
                break;
        }
    }

    connection.sendCallResponse(DeleteCertificateResponse{uid, status});
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleGetInstalledCertificateIds(const char *uid, GetInstalledCertificateIdsView req)
{
    // Requested types, all when the filter is absent (M03.FR.05).
    bool want[6] = {false, false, false, false, false, false};
    size_t filter_count = req.certificateType_count();
    if (filter_count == 0) {
        for (auto &w : want) {
            w = true;
        }
    } else {
        for (size_t i = 0; i < filter_count; ++i) {
            auto t = req.certificateType(i);
            if (t.is_some()) {
                want[(size_t)t.unwrap()] = true;
            }
        }
    }

    auto entry_type = [](CertGroup group) -> GetInstalledCertificateIdsCertificateTypeEntry {
        switch (group) {
            case CertGroup::V2GRoot:    return GetInstalledCertificateIdsCertificateTypeEntry::V2_G_ROOT_CERTIFICATE;
            case CertGroup::MORoot:     return GetInstalledCertificateIdsCertificateTypeEntry::MO_ROOT_CERTIFICATE;
            case CertGroup::OEMRoot:    return GetInstalledCertificateIdsCertificateTypeEntry::OEM_ROOT_CERTIFICATE;
            case CertGroup::CsmsRoot:   return GetInstalledCertificateIdsCertificateTypeEntry::CSMS_ROOT_CERTIFICATE;
            case CertGroup::MfrRoot:    return GetInstalledCertificateIdsCertificateTypeEntry::MANUFACTURER_ROOT_CERTIFICATE;
            case CertGroup::V2GChain:
            case CertGroup::V2G20Chain: return GetInstalledCertificateIdsCertificateTypeEntry::V2_G_CERTIFICATE_CHAIN;
            default:                      return (GetInstalledCertificateIdsCertificateTypeEntry)6;
        }
    };

    size_t count = 0;
    for (auto &e : cert_store.all()) {
        auto t = entry_type(e.group);
        if ((size_t)t < 6 && want[(size_t)t]) {
            ++count;
        }
    }

    if (count == 0) {
        connection.sendCallResponse(GetInstalledCertificateIdsResponse{uid, GetInstalledCertificateIdsResponseStatus::NOT_FOUND});
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    auto chains = heap_alloc_array<GetInstalledCertificateIdsResponseCertificateHashDataChain>(count);
    auto hashes = heap_alloc_array<GetInstalledCertificateIdsResponseCertificateHashDataChainCertificateHashData>(count);
    auto children = heap_alloc_array<GetInstalledCertificateIdsResponseCertificateHashDataChainChildCertificateHashData>(count * OCPP21_CHAIN_MAX_CHILDREN);

    size_t i = 0;
    for (auto &e : cert_store.all()) {
        auto t = entry_type(e.group);
        if ((size_t)t >= 6 || !want[(size_t)t]) {
            continue;
        }

        hashes[i].hashAlgorithm = HashAlgorithm::SHA256;
        hashes[i].issuerNameHash = e.hash.issuer_name_hash;
        hashes[i].issuerKeyHash = e.hash.issuer_key_hash;
        hashes[i].serialNumber = e.hash.serial_number;

        chains[i].certificateHashData = &hashes[i];
        chains[i].certificateType = (GetInstalledCertificateIdsResponseCertificateHashDataChainCertificateType)(size_t)t;

        // HUB20-412-001: sub CAs in chain order, CPO sub CA 2 first.
        if (e.child_count > 0) {
            auto *child = &children[i * OCPP21_CHAIN_MAX_CHILDREN];
            for (size_t c = 0; c < e.child_count; ++c) {
                child[c].hashAlgorithm = HashAlgorithm::SHA256;
                child[c].issuerNameHash = e.child_hash[c].issuer_name_hash;
                child[c].issuerKeyHash = e.child_hash[c].issuer_key_hash;
                child[c].serialNumber = e.child_hash[c].serial_number;
            }
            chains[i].childCertificateHashData = child;
            chains[i].childCertificateHashData_length = e.child_count;
        }
        ++i;
    }

    connection.sendCallResponse(GetInstalledCertificateIdsResponse{uid, GetInstalledCertificateIdsResponseStatus::ACCEPTED, nullptr, chains.get(), count});
    return CallResponse{CallErrorCode::OK, nullptr};
}

CallResponse ChargePoint::handleSetNetworkProfile(const char *uid, SetNetworkProfileView req)
{
    auto reject = [this, uid]() {
        connection.sendCallResponse(SetNetworkProfileResponse{uid, eResponseStatus::REJECTED});
        return CallResponse{CallErrorCode::OK, nullptr};
    };

    int32_t slot_num = req.configurationSlot();
    if (slot_num < 1 || slot_num > OCPP21_NETWORK_PROFILE_SLOTS) {
        return reject();
    }

    auto cd = req.connectionData();
    if (cd.ocppTransport() != SetNetworkProfileConnectionDataEntriesOcppTransport::JSON) {
        return reject();
    }
    if (cd.ocppVersion().is_some() && cd.ocppVersion().unwrap() != SetNetworkProfileConnectionDataEntriesOcppVersion::OCPP21) {
        return reject();
    }
    // No VPN or APN backed network interfaces.
    if (cd.vpn().is_some() || cd.apn().is_some()) {
        return reject();
    }
    if (cd.identity().is_some() && strcmp(cd.identity().unwrap(), charge_point_name.c_str()) != 0) {
        return reject();
    }

    // AllowSecurityProfileDowngrade is not supported (reported absent),
    // a lower security profile is always rejected (A05.FR.03).
    int32_t sp = cd.securityProfile();
    if (sp < 1 || sp > 3 || sp < device_model.security_profile) {
        return reject();
    }

    const char *url = cd.ocppCsmsUrl();
    if (strlen(url) > OCPP21_NETWORK_PROFILE_URL_MAX_LEN) {
        return reject();
    }
    bool is_wss = strncmp(url, "wss://", 6) == 0;
    if (sp == 1 ? !((strncmp(url, "ws://", 5) == 0) || is_wss) : !is_wss) {
        return reject();
    }

    const char *pass = "";
    if (cd.basicAuthPassword().is_some()) {
        pass = cd.basicAuthPassword().unwrap();
        size_t pass_len = strlen(pass);
        if (pass_len < OCPP21_BASIC_AUTH_PASSWORD_MIN_LEN || pass_len > OCPP21_BASIC_AUTH_PASSWORD_MAX_LEN) {
            return reject();
        }
    }

    // The profile must be usable: TLS needs a CSMS root to verify against,
    // profile 3 needs a charging station certificate.
    if (sp >= 2 && tls_ca_file.empty() && cert_store.find(CertGroup::CsmsRoot) == nullptr) {
        return reject();
    }
    if (sp == 3 && tls_client_cert_file.empty() && cert_store.find(CertGroup::CsmsClientChain) == nullptr) {
        return reject();
    }

    auto &slot = device_model.network_profiles[slot_num - 1];
    slot.used = true;
    slot.security_profile = sp;
    snprintf(slot.url, sizeof(slot.url), "%s", url);
    snprintf(slot.password, sizeof(slot.password), "%s", pass);
    saveNetworkPersistence();

    log_info("Stored network profile in slot %d (security profile %d)", slot_num, sp);
    connection.sendCallResponse(SetNetworkProfileResponse{uid, eResponseStatus::ACCEPTED});
    return CallResponse{CallErrorCode::OK, nullptr};
}

void ChargePoint::applyNetworkProfile()
{
    auto &slot = device_model.network_profiles[device_model.network_priority - 1];

    PlatformTlsConfig tls;
    bool use_tls = slot.security_profile >= 2;
    if (use_tls) {
        if (tls_ca_file.empty()) {
            const CertEntry *root = cert_store.find(CertGroup::CsmsRoot);
            if (root == nullptr) {
                log_error("Network profile %d needs a CSMS root certificate", device_model.network_priority);
                return;
            }
            tls_ca_file = cert_store.pemPath(CertGroup::CsmsRoot, root->id);
        }
        if (slot.security_profile == 3 && tls_client_cert_file.empty()) {
            const CertEntry *chain = cert_store.find(CertGroup::CsmsClientChain);
            if (chain == nullptr) {
                log_error("Network profile %d needs a charging station certificate", device_model.network_priority);
                return;
            }
            tls_client_cert_file = cert_store.pemPath(CertGroup::CsmsClientChain, chain->id);
            tls_client_key_file = cert_store.keyPath(chain->id);
        }

        tls.ca_cert_file = tls_ca_file.c_str();
        if (slot.security_profile == 3) {
            tls.client_cert_file = tls_client_cert_file.empty() ? nullptr : tls_client_cert_file.c_str();
            tls.client_key_file = tls_client_key_file.empty() ? nullptr : tls_client_key_file.c_str();
        }
    }
    tls_in_use = use_tls;
    device_model.security_profile = slot.security_profile;

    const char *pass = slot.password[0] != '\0' ? slot.password : device_model.basic_auth_password;
    log_info("Connecting with network profile slot %d (security profile %d)", device_model.network_priority, slot.security_profile);
    connection.updateEndpoint(slot.url, pass, use_tls ? &tls : nullptr);
}

#define OCPP21_NETWORK_PERSISTENCE_BUF_LEN 1280

void ChargePoint::loadNetworkPersistence()
{
    std::string name = charge_point_name + ".netprof21";
    char buf[OCPP21_NETWORK_PERSISTENCE_BUF_LEN];
    size_t len = platform_read_file(name.c_str(), buf, sizeof(buf) - 1);
    if (len == 0) {
        return;
    }
    buf[len] = '\0';

    StaticJsonDocument<OCPP21_NETWORK_PERSISTENCE_BUF_LEN * 2> doc;
    if (deserializeJson(doc, buf, len)) {
        log_error("Failed to parse %s", name.c_str());
        return;
    }

    for (JsonObject s : doc["slots"].as<JsonArray>()) {
        int32_t slot_num = s["slot"].as<int32_t>();
        if (slot_num < 1 || slot_num > OCPP21_NETWORK_PROFILE_SLOTS) {
            continue;
        }
        auto &slot = device_model.network_profiles[slot_num - 1];
        slot.used = true;
        slot.security_profile = s["security_profile"].as<int32_t>();
        snprintf(slot.url, sizeof(slot.url), "%s", s["url"].as<const char *>());
        snprintf(slot.password, sizeof(slot.password), "%s", s["password"].as<const char *>());
    }

    int32_t priority = doc["priority"].as<int32_t>();
    if (priority >= 1 && priority <= OCPP21_NETWORK_PROFILE_SLOTS && device_model.network_profiles[priority - 1].used) {
        device_model.network_priority = priority;
    }
}

void ChargePoint::saveNetworkPersistence()
{
    std::string name = charge_point_name + ".netprof21";
    char buf[OCPP21_NETWORK_PERSISTENCE_BUF_LEN];
    TFJsonSerializer json{buf, sizeof(buf)};
    json.addObject();
    json.addMemberNumber("priority", device_model.network_priority);
    json.addMemberArray("slots");
    for (int32_t i = 0; i < OCPP21_NETWORK_PROFILE_SLOTS; ++i) {
        auto &slot = device_model.network_profiles[i];
        if (!slot.used) {
            continue;
        }
        json.addObject();
        json.addMemberNumber("slot", i + 1);
        json.addMemberNumber("security_profile", slot.security_profile);
        json.addMemberString("url", slot.url);
        json.addMemberString("password", slot.password);
        json.endObject();
    }
    json.endArray();
    json.endObject();
    size_t len = json.end();

    if (!platform_write_file(name.c_str(), buf, len)) {
        log_error("Failed to write %s", name.c_str());
    }
}

void ChargePoint::scheduleChainOcsp(uint32_t chain_id)
{
    const CertEntry *e = cert_store.findSeccChainById(chain_id);
    if (e == nullptr) {
        return;
    }

    for (auto &slot : ocsp_cache) {
        if (slot.used && slot.chain_id == chain_id) {
            slot.clear();
        }
    }

    if (!e->has_anchor) {
        return;
    }

    auto pem = heap_alloc_array<char>(OCPP21_CERT_PEM_MAX + 1);
    if (cert_store.readPem(*e, pem.get(), OCPP21_CERT_PEM_MAX + 1) == 0) {
        return;
    }

    size_t cert_count = platform_cert_count21(pem.get());
    for (size_t idx = 0; idx < cert_count; ++idx) {
        char url[256];
        if (!platform_cert_ocsp_url21(pem.get(), idx, url, sizeof(url))) {
            continue;
        }
        for (auto &slot : ocsp_cache) {
            if (slot.used) {
                continue;
            }
            slot.used = true;
            slot.in_flight = false;
            slot.chain_id = chain_id;
            slot.cert_idx = (uint8_t)idx;
            slot.hash = idx == 0 ? e->hash : e->child_hash[idx - 1];
            snprintf(slot.url, sizeof(slot.url), "%s", url);
            slot.status = OcppOcspStatus21::Unknown;
            slot.refresh_deadline = set_deadline(0);
            break;
        }
    }
}

CallResponse ChargePoint::handleGetCertificateStatusResponse(int32_t connectorId, GetCertificateStatusResponseView conf)
{
    (void)connectorId;
    if (ocsp_in_flight_idx < 0) {
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    auto &slot = ocsp_cache[ocsp_in_flight_idx];
    ocsp_in_flight_idx = -1;
    slot.in_flight = false;
    slot.refresh_deadline = set_deadline(OCPP21_OCSP_RETRY_MS);

    if (conf.status() != GetCertificateStatusResponseStatus::ACCEPTED || conf.ocspResult().is_none()) {
        log_warn("GetCertificateStatus failed, retrying later");
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    const CertEntry *e = cert_store.findSeccChainById(slot.chain_id);
    if (e == nullptr) {
        slot.used = false;
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    auto pem = heap_alloc_array<char>(OCPP21_CERT_PEM_MAX + 1);
    if (cert_store.readPem(*e, pem.get(), OCPP21_CERT_PEM_MAX + 1) == 0) {
        return CallResponse{CallErrorCode::OK, nullptr};
    }

    size_t cert_count = platform_cert_count21(pem.get());
    std::string root_pem = cert_store.loadRootByHash(e->anchor_root);
    const char *issuer_pem = pem.get();
    size_t issuer_idx = slot.cert_idx + 1;
    if ((size_t)slot.cert_idx == cert_count - 1) {
        if (root_pem.empty()) {
            return CallResponse{CallErrorCode::OK, nullptr};
        }
        issuer_pem = root_pem.c_str();
        issuer_idx = 0;
    }

    std::unique_ptr<char[]> root_bufs[OCPP21_CERTSTORE_MAX_V2G_ROOT];
    const char *root_ptrs[OCPP21_CERTSTORE_MAX_V2G_ROOT];
    size_t roots = cert_store.loadRoots(CertGroup::V2GRoot, root_bufs, root_ptrs, OCPP21_CERTSTORE_MAX_V2G_ROOT);

    time_t now = platform_get_system_time(connection.platform_ctx);
    time_t next_update = 0;

    // Retain the raw response of -20 chain certificates for TLS 1.3
    // stapling (V2G20-2388). The DER is shorter than its base64 form.
    bool retain = e->group == CertGroup::V2G20Chain;
    std::unique_ptr<uint8_t[]> der_buf;
    size_t der_cap = 0;
    size_t der_len = 0;
    if (retain) {
        der_cap = strlen(conf.ocspResult().unwrap());
        der_buf = heap_alloc_array<uint8_t>(der_cap);
    }

    OcppOcspStatus21 old_status = slot.status;
    auto result = platform_ocsp_validate21(conf.ocspResult().unwrap(), pem.get(), slot.cert_idx,
                                           issuer_pem, issuer_idx, root_ptrs, roots, now, &next_update,
                                           der_buf.get(), der_cap, retain ? &der_len : nullptr);
    slot.status = result;

    bool staple_changed = false;
    if (result == OcppOcspStatus21::Good && der_len > 0) {
        staple_changed = (slot.response_der_len != der_len) || (memcmp(slot.response_der.get(), der_buf.get(), der_len) != 0);
        if (staple_changed) {
            slot.response_der = heap_alloc_array<uint8_t>(der_len);
            memcpy(slot.response_der.get(), der_buf.get(), der_len);
            slot.response_der_len = der_len;
        }
    }

    switch (result) {
        case OcppOcspStatus21::Good: {
            // HUB20-431-001: refresh at nextUpdate or after 7 days,
            // whichever comes first (M06.FR.10).
            time_t delta = next_update > now ? next_update - now : OCPP21_OCSP_MAX_CACHE_S;
            if (delta > OCPP21_OCSP_MAX_CACHE_S) {
                delta = OCPP21_OCSP_MAX_CACHE_S;
            }
            slot.refresh_deadline = set_deadline((uint32_t)delta * 1000);
            log_info("OCSP status good for chain certificate %u/%u", slot.chain_id, slot.cert_idx);
            break;
        }
        case OcppOcspStatus21::Revoked: {
            // HUB20-431-003: delete the SECC chain immediately.
            log_warn("OCSP status revoked, deleting the SECC chain");
            uint32_t chain_id = slot.chain_id;
            cert_store.removeChain(e->group, chain_id);
            for (auto &s : ocsp_cache) {
                if (s.used && s.chain_id == chain_id) {
                    s.clear();
                }
            }
            platform_cert_store_changed21(connection.platform_ctx);
            break;
        }
        case OcppOcspStatus21::Unknown:
            log_warn("OCSP status unknown, retrying later");
            break;
        case OcppOcspStatus21::Invalid:
            // HUB20-431-002: reject responses that fail validation.
            log_warn("OCSP response failed validation, rejected");
            break;
    }

    // TLS 1.3 availability and the stapled data follow this cache
    // (HUB20-532-002, V2G20-2388), trigger a reload in the ISO 15118
    // stack. The Revoked path notified via the chain deletion already.
    if (result != OcppOcspStatus21::Revoked && (result != old_status || staple_changed)) {
        platform_cert_store_changed21(connection.platform_ctx);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

OcppOcspStatus21 ChargePoint::seccChainOcspStatus(uint32_t chain_id) const
{
    bool any = false;
    for (const auto &slot : ocsp_cache) {
        if (!slot.used || slot.chain_id != chain_id) {
            continue;
        }
        if (slot.status == OcppOcspStatus21::Revoked) {
            return OcppOcspStatus21::Revoked;
        }
        if (slot.status != OcppOcspStatus21::Good) {
            return OcppOcspStatus21::Unknown;
        }
        any = true;
    }
    return any ? OcppOcspStatus21::Good : OcppOcspStatus21::Unknown;
}

bool ChargePoint::seccChainOcspResponse(uint32_t chain_id, uint8_t cert_idx, const uint8_t **der, size_t *der_len) const
{
    for (const auto &slot : ocsp_cache) {
        if (slot.used && slot.chain_id == chain_id && slot.cert_idx == cert_idx && slot.response_der_len > 0) {
            *der = slot.response_der.get();
            *der_len = slot.response_der_len;
            return true;
        }
    }
    return false;
}

bool ChargePoint::requestVehicleChainStatus(const OcppCertHashData21 *hashes, const char * const *responder_urls, size_t count)
{
    // M07 plumbing for the ISO 15118 stack (HUB20-432-001). The caller
    // passes the full chain leaf first, i.e. Leaf, Sub2, Sub1, which is
    // the wire order required by HUB20-432-006 (and HUB20-432-005/007,
    // hashes for all chain certificates in every request).
    if (count == 0 || count > OCPP21_VEHICLE_OCSP_CACHE_SIZE || vehicle_status_in_flight) {
        return false;
    }

    GetCertificateChainStatusCertificateStatusRequests requests[OCPP21_VEHICLE_OCSP_CACHE_SIZE];
    GetCertificateChainStatusCertificateStatusRequestsCertificateHashData hash_data[OCPP21_VEHICLE_OCSP_CACHE_SIZE];
    const char *urls[OCPP21_VEHICLE_OCSP_CACHE_SIZE];

    for (size_t i = 0; i < count; ++i) {
        hash_data[i].hashAlgorithm = HashAlgorithm::SHA256;
        hash_data[i].issuerNameHash = hashes[i].issuer_name_hash;
        hash_data[i].issuerKeyHash = hashes[i].issuer_key_hash;
        hash_data[i].serialNumber = hashes[i].serial_number;
        urls[i] = responder_urls[i];
        requests[i].certificateHashData = &hash_data[i];
        requests[i].source = GetCertificateChainStatusCertificateStatusRequestsSource::OCSP;
        requests[i].urls = &urls[i];
        requests[i].urls_length = 1;
    }

    vehicle_status_in_flight = connection.sendCallAction(GetCertificateChainStatus{requests, count});
    return vehicle_status_in_flight;
}

const VehicleOcspStatus *ChargePoint::vehicleChainStatus(const OcppCertHashData21 &hash) const
{
    for (auto &v : vehicle_ocsp) {
        if (v.used && strcasecmp(v.hash.serial_number, hash.serial_number) == 0
         && strcasecmp(v.hash.issuer_key_hash, hash.issuer_key_hash) == 0
         && strcasecmp(v.hash.issuer_name_hash, hash.issuer_name_hash) == 0) {
            return &v;
        }
    }
    return nullptr;
}

CallResponse ChargePoint::handleGetCertificateChainStatusResponse(int32_t connectorId, GetCertificateChainStatusResponseView conf)
{
    (void)connectorId;
    size_t count = conf.certificateStatus_count();
    time_t now = platform_get_system_time(connection.platform_ctx);

    for (size_t i = 0; i < count; ++i) {
        auto status = conf.certificateStatus(i);
        auto hash = status.certificateHashData();

        // Cache entries are matched by hash data, the response order is
        // not guaranteed to mirror the request.
        VehicleOcspStatus *slot = nullptr;
        for (auto &v : vehicle_ocsp) {
            if (v.used && strcasecmp(v.hash.serial_number, hash.serialNumber()) == 0
             && strcasecmp(v.hash.issuer_key_hash, hash.issuerKeyHash()) == 0
             && strcasecmp(v.hash.issuer_name_hash, hash.issuerNameHash()) == 0) {
                slot = &v;
                break;
            }
        }
        if (slot == nullptr) {
            for (auto &v : vehicle_ocsp) {
                if (!v.used || v.next_update <= now) {
                    slot = &v;
                    break;
                }
            }
        }
        if (slot == nullptr) {
            slot = &vehicle_ocsp[i % OCPP21_VEHICLE_OCSP_CACHE_SIZE];
        }

        slot->used = true;
        snprintf(slot->hash.issuer_name_hash, sizeof(slot->hash.issuer_name_hash), "%s", hash.issuerNameHash());
        snprintf(slot->hash.issuer_key_hash, sizeof(slot->hash.issuer_key_hash), "%s", hash.issuerKeyHash());
        snprintf(slot->hash.serial_number, sizeof(slot->hash.serial_number), "%s", hash.serialNumber());
        slot->status = status.status();
        // M07.FR.09: cache at most one week.
        slot->next_update = status.nextUpdate();
        if (slot->next_update > now + OCPP21_OCSP_MAX_CACHE_S) {
            slot->next_update = now + OCPP21_OCSP_MAX_CACHE_S;
        }
        log_info("Vehicle chain certificate %s: OCSP status %s", slot->hash.serial_number,
                 GetCertificateChainStatusResponseCertificateStatusEntryEntriesStatusStrings[(size_t)slot->status]);
    }

    if (vehicle_status_in_flight) {
        vehicle_status_in_flight = false;
        platform_vehicle_chain_status_result21(connection.platform_ctx, true);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

bool ChargePoint::request15118EVCertificate(const char *iso15118_schema_version, bool update, const char *exi_request,
                                            int32_t maximum_contract_certificate_chains,
                                            const char **prioritized_emaids, size_t prioritized_emaids_count)
{
    // M01/M02, gated by ISO15118Ctrlr Enabled and
    // ContractCertificateInstallationEnabled.
    if (!device_model.iso15118_pnc_supported || !device_model.iso15118_enabled
     || !device_model.contract_cert_install_enabled || ev_cert_in_flight) {
        return false;
    }

    if (!connection.sendCallAction(Get15118EVCertificate{iso15118_schema_version,
                                                         update ? Get15118EVCertificateAction::UPDATE : Get15118EVCertificateAction::INSTALL,
                                                         exi_request,
                                                         maximum_contract_certificate_chains >= 0 ? maximum_contract_certificate_chains : OCPP_INTEGER_NOT_PASSED,
                                                         prioritized_emaids, prioritized_emaids_count})) {
        return false;
    }

    ev_cert_in_flight = true;
    return true;
}

void ChargePoint::register15118EVCertificateResult(void (*cb)(bool accepted, const char *exi_response, int32_t remaining_contracts, void *user_data), void *user_data)
{
    ev_cert_result_cb = cb;
    ev_cert_result_user_data = user_data;
}

CallResponse ChargePoint::handleGet15118EVCertificateResponse(int32_t connectorId, Get15118EVCertificateResponseView conf)
{
    (void)connectorId;
    if (!ev_cert_in_flight) {
        return CallResponse{CallErrorCode::OK, nullptr};
    }
    ev_cert_in_flight = false;

    bool accepted = conf.status() == GetCertificateStatusResponseStatus::ACCEPTED;
    int32_t remaining = conf.remainingContracts().is_some() ? conf.remainingContracts().unwrap() : 0;
    log_info("Get15118EVCertificate %s, %d contracts remaining", accepted ? "accepted" : "failed", remaining);

    if (ev_cert_result_cb != nullptr) {
        ev_cert_result_cb(accepted, accepted ? conf.exiResponse() : nullptr, remaining, ev_cert_result_user_data);
    }

    return CallResponse{CallErrorCode::OK, nullptr};
}

} // namespace Ocpp21
