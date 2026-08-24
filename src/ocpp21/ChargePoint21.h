#pragma once

#include <time.h>

#include <string>

#include "CertStore21.h"
#include "Connection21.h"
#include "DeviceModel21.h"
#include "Messages21.h"
#include "Platform21.h"
#include "Types21.h"

namespace Ocpp21 {

enum class State {
    PowerOn,  // boot notification not yet accepted
    Pending,  // boot notification answered with Pending
    Rejected, // boot notification answered with Rejected
    Idle,     // boot notification accepted
};

#define OCPP21_ID_TOKEN_LEN 64
#define OCPP21_ID_TOKEN_TYPE_LEN 20
#define OCPP21_TRANSACTION_ID_LEN 36

// Per EVSE connector and transaction state. One connector per EVSE,
// TxStartPoint and TxStopPoint fixed to PowerPathClosed (simplified to
// authorized and cable plugged).
struct EvseTracker {
    EvseState21 last_state = EvseState21::NotConnected;
    StatusNotificationConnectorStatus last_sent_status = StatusNotificationConnectorStatus::NONE;

    // Authorization waiting for or attached to a transaction.
    bool authorized = false;
    char id_token[OCPP21_ID_TOKEN_LEN + 1] = {};
    char id_token_type[OCPP21_ID_TOKEN_TYPE_LEN + 1] = {};
    bool remote_start = false;
    int32_t remote_start_id = 0;
    // Trigger reason for the Started event, set when the authorization is granted.
    TransactionEventTriggerReason start_trigger = TransactionEventTriggerReason::NONE;
    // EVConnectionTimeOut while authorized but no cable plugged.
    uint32_t ev_connect_deadline = 0;

    bool waiting_for_tag = false;
    uint32_t tag_deadline = 0;
    bool tag_window_closed = false;

    // Active transaction.
    bool transaction_active = false;
    char transaction_id[OCPP21_TRANSACTION_ID_LEN + 1] = {};
    int32_t seq_no = 0;
    TransactionEventTransactionInfoChargingState charging_state = TransactionEventTransactionInfoChargingState::NONE;
    uint32_t next_sampled_value_deadline = 0;
};

// M06/M07: cached OCSP status of a SECC chain certificate.
struct OcspCacheEntry {
    bool used = false;
    bool in_flight = false;
    uint32_t chain_id = 0;
    uint8_t cert_idx = 0;
    OcppCertHashData21 hash{};
    char url[256] = "";
    OcppOcspStatus21 status = OcppOcspStatus21::Unknown;
    uint32_t refresh_deadline = 0;
};

#define OCPP21_OCSP_CACHE_SIZE (OCPP21_CERTSTORE_MAX_CHAINS * (OCPP21_CHAIN_MAX_CHILDREN + 1))

// M07: vehicle chain status cache, filled from
// GetCertificateChainStatusResponse. Read by the ISO 15118 stack.
struct VehicleOcspStatus {
    bool used = false;
    OcppCertHashData21 hash{};
    GetCertificateChainStatusResponseCertificateStatusEntryEntriesStatus status = GetCertificateChainStatusResponseCertificateStatusEntryEntriesStatus::UNKNOWN;
    time_t next_update = 0;
};

#define OCPP21_VEHICLE_OCSP_CACHE_SIZE 4

class ChargePoint {
public:
    ChargePoint() {}

    ChargePoint(const ChargePoint&) = delete;
    ChargePoint &operator=(const ChargePoint&) = delete;

    bool start(const char *websocket_endpoint_url, const char *charge_point_name, const char *basic_auth_pass, int32_t security_profile = 1, const PlatformTlsConfig *tls = nullptr);
    void stop();
    void tick();

    // Connection events
    void onConnect();
    void onDisconnect();
    void onTimeout(CallAction action, uint64_t messageId);
    void onCallError(CallAction action, uint64_t messageId);
    void onConnectionError(PlatformConnectionError error);

    // Platform events
    void onTagSeen(int32_t evse_id, const char *tag_id);

    // Queues a critical security event for guaranteed delivery (A04).
    void sendSecurityEventNotification(const char *type, const char *tech_info = nullptr);

    // M07: requests the OCSP status of a vehicle certificate chain
    // (hashes with per certificate responder URL). The result lands in
    // the vehicle status cache, see vehicleChainStatus.
    bool requestVehicleChainStatus(const OcppCertHashData21 *hashes, const char * const *responder_urls, size_t count);
    const VehicleOcspStatus *vehicleChainStatus(const OcppCertHashData21 &hash) const;

    // Received call results
    CallResponse handleBootNotificationResponse(int32_t connectorId, BootNotificationResponseView conf);
    CallResponse handleHeartbeatResponse(int32_t connectorId, HeartbeatResponseView conf);
    CallResponse handleStatusNotificationResponse(int32_t connectorId, StatusNotificationResponseView conf);
    CallResponse handleAuthorizeResponse(int32_t connectorId, AuthorizeResponseView conf);
    CallResponse handleTransactionEventResponse(int32_t connectorId, TransactionEventResponseView conf);
    CallResponse handleMeterValuesResponse(int32_t connectorId, MeterValuesResponseView conf);
    CallResponse handleSecurityEventNotificationResponse(int32_t connectorId, SecurityEventNotificationResponseView conf);
    CallResponse handleSignCertificateResponse(int32_t connectorId, SignCertificateResponseView conf);
    CallResponse handleGetCertificateStatusResponse(int32_t connectorId, GetCertificateStatusResponseView conf);
    CallResponse handleGetCertificateChainStatusResponse(int32_t connectorId, GetCertificateChainStatusResponseView conf);

    // Received calls
    CallResponse handleGetVariables(const char *uid, GetVariablesView req);
    CallResponse handleSetVariables(const char *uid, SetVariablesView req);
    CallResponse handleGetBaseReport(const char *uid, GetBaseReportView req);
    CallResponse handleTriggerMessage(const char *uid, TriggerMessageView req);
    CallResponse handleReset(const char *uid, ResetView req);
    CallResponse handleRequestStartTransaction(const char *uid, RequestStartTransactionView req);
    CallResponse handleRequestStopTransaction(const char *uid, RequestStopTransactionView req);
    CallResponse handleCertificateSigned(const char *uid, CertificateSignedView req);
    CallResponse handleInstallCertificate(const char *uid, InstallCertificateView req);
    CallResponse handleDeleteCertificate(const char *uid, DeleteCertificateView req);
    CallResponse handleGetInstalledCertificateIds(const char *uid, GetInstalledCertificateIdsView req);
    CallResponse handleSetNetworkProfile(const char *uid, SetNetworkProfileView req);

    State state = State::PowerOn;

    Connection connection;
    DeviceModel device_model;
    CertStore cert_store;

private:
    void sendBootNotification();
    void sendStatusNotifications();
    void tickEvses();
    void startTransaction(int32_t evse_id, TransactionEventTriggerReason trigger);
    void stopTransaction(int32_t evse_id, TransactionEventTriggerReason trigger, TransactionEventTransactionInfoStoppedReason reason, bool include_token);
    void sendTransactionUpdated(int32_t evse_id, TransactionEventTriggerReason trigger, bool with_meter_value);
    void loadSecurityPersistence();
    void saveSecurityPersistence();
    void loadNetworkPersistence();
    void saveNetworkPersistence();
    void applyNetworkProfile();

    void startCsr(SignCertificateCertificateType type, bool renewal, const OcppCertHashData21 *root_hash);
    void abortCsr();
    void sendSignCertificate();
    void tickCertificates();
    void applyClientCertificate(uint32_t chain_id);
    // M06: (re)creates the OCSP cache entries for a SECC chain.
    void scheduleChainOcsp(uint32_t chain_id);

    uint32_t boot_retry_deadline = 0;
    // Interval requested by the CSMS in a Pending or Rejected boot response. 0 = use default.
    int32_t boot_retry_interval_s = 0;
    bool boot_notification_in_flight = false;

    uint32_t next_heartbeat_deadline = 0;

    bool status_notifications_pending = false;

    bool trigger_boot_notification = false;
    bool trigger_heartbeat = false;
    bool trigger_status_notification = false;

    EvseTracker evses[1];

    // A tag swipe reported by the platform, handled in the next tick.
    bool tag_pending = false;
    int32_t tag_evse_id = 0;
    char pending_tag[OCPP21_ID_TOKEN_LEN + 1] = {};

    // Only one Authorize is in flight at a time. authorize_for_stop marks
    // an Authorize sent to stop a running transaction with a different
    // token, the 1.6 AUTH_STOP flow.
    bool authorize_in_flight = false;
    bool authorize_for_stop = false;
    int32_t authorize_evse_id = 0;
    char authorize_token[OCPP21_ID_TOKEN_LEN + 1] = {};

    // Also used as the persistence file name prefix.
    std::string charge_point_name;

    // A01: reconnect with the new BasicAuthPassword after the SetVariablesResponse left. 0 = not armed.
    uint32_t password_reconnect_deadline = 0;

    // A05: switch to the active network profile after the SetVariablesResponse left. 0 = not armed.
    uint32_t network_reconnect_deadline = 0;

    // Report a TLS failure security event once per failure streak
    // (A00.FR.316). Unknown doubles as the none sentinel, reset on connect.
    PlatformConnectionError last_reported_conn_error = PlatformConnectionError::Unknown;

    // A02/A03: one CSR flow at a time. The CSR is kept for resends
    // (A02.FR.18), the retry backoff starts at CertSigningWaitMinimum
    // and doubles (A02.FR.17/18), stopping after CertSigningRepeatTimes
    // resends (A02.FR.19).
    bool csr_active = false;
    SignCertificateCertificateType csr_type = SignCertificateCertificateType::NONE;
    uint32_t csr_pending_id = 0; // reserved store id, names the key file
    int32_t csr_request_id = 0;  // A02.FR.24/26
    int32_t last_sign_request_id = 0;
    char csr_buf[5501] = "";
    uint32_t csr_retry_deadline = 0;
    int32_t csr_attempts_left = 0;
    uint32_t csr_backoff_s = 0;
    bool csr_has_root_hash = false; // A03.FR.23, omitted for A02 (HUB20-421-002)
    OcppCertHashData21 csr_root_hash{};

    // TriggerMessage requested CSR, started from the next tick so the
    // TriggerMessageResponse leaves first.
    bool trigger_sign = false;
    SignCertificateCertificateType trigger_sign_type = SignCertificateCertificateType::NONE;

    // A03.FR.02: periodic check for certificates within one month of expiry.
    uint32_t cert_expiry_check_deadline = 0;

    // A02.FR.08: reconnect with the new charging station certificate
    // after the CertificateSignedResponse left. 0 = not armed.
    uint32_t cert_reconnect_deadline = 0;
    uint32_t pending_client_chain_id = 0;

    // TLS configuration in use, needed to swap the client certificate.
    bool tls_in_use = false;
    std::string tls_ca_file;
    std::string tls_client_cert_file;
    std::string tls_client_key_file;

    // M06: SECC chain OCSP status cache. One request in flight at a time.
    OcspCacheEntry ocsp_cache[OCPP21_OCSP_CACHE_SIZE];
    int32_t ocsp_in_flight_idx = -1;

    // M07 plumbing.
    VehicleOcspStatus vehicle_ocsp[OCPP21_VEHICLE_OCSP_CACHE_SIZE];
};

} // namespace Ocpp21
