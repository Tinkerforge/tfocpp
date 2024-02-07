import struct

NUM_CONNECTORS = 1

# charge point state
# statusnotificationstatus
# message if type
# connector state
# tag status
# configuration

# keep in sync with ChargePoint.h OcppStateStrings
charge_point_state_strings = [
    "PowerOn",
    "FlushPersistentMessages",
    "Idle",
    "Pending",
    "Rejected",
    "Unavailable",
    "Faulted",
    "SoftReset",
    "HardReset"
]

# TODO: generate
status_notification_status_strings = [
    "Available",
    "Preparing",
    "Charging",
    "SuspendedEV",
    "SuspendedEVSE",
    "Finishing",
    "Reserved",
    "Unavailable",
    "Faulted",
    "NONE"
]

#TODO generate
message_in_flight_type_strings = [
    "Authorize",
    "BootNotification",
    "ChangeAvailabilityResponse",
    "ChangeConfigurationResponse",
    "ClearCacheResponse",
    "DataTransfer",
    "DataTransferResponse",
    "GetConfigurationResponse",
    "Heartbeat",
    "MeterValues",
    "RemoteStartTransactionResponse",
    "RemoteStopTransactionResponse",
    "ResetResponse",
    "StartTransaction",
    "StatusNotification",
    "StopTransaction",
    "UnlockConnectorResponse",
    "AuthorizeResponse",
    "BootNotificationResponse",
    "ChangeAvailability",
    "ChangeConfiguration",
    "ClearCache",
    "GetConfiguration",
    "HeartbeatResponse",
    "MeterValuesResponse",
    "RemoteStartTransaction",
    "RemoteStopTransaction",
    "Reset",
    "StartTransactionResponse",
    "StatusNotificationResponse",
    "StopTransactionResponse",
    "UnlockConnector",
    "GetDiagnosticsResponse",
    "DiagnosticsStatusNotification",
    "FirmwareStatusNotification",
    "UpdateFirmwareResponse",
    "GetDiagnostics",
    "DiagnosticsStatusNotificationResponse",
    "FirmwareStatusNotificationResponse",
    "UpdateFirmware",
    "GetLocalListVersionResponse",
    "SendLocalListResponse",
    "GetLocalListVersion",
    "SendLocalList",
    "CancelReservationResponse",
    "ReserveNowResponse",
    "CancelReservation",
    "ReserveNow",
    "ClearChargingProfileResponse",
    "GetCompositeScheduleResponse",
    "SetChargingProfileResponse",
    "ClearChargingProfile",
    "GetCompositeSchedule",
    "SetChargingProfile",
    "TriggerMessageResponse",
    "TriggerMessage"
]

# keep in sync with Connector.cpp ConnectorStateStrings
connector_state_strings = [
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
]

# TODO: generate
tag_status_strings = [
    "Accepted",
    "Blocked",
    "Expired",
    "Invalid",
    "ConcurrentTx"
]

# keep in sync with Configuration config_keys
config_key_strings = [
    # CORE PROFILE
    #"AllowOfflineTxForUnknownId",
    #"AuthorizationCacheEnabled",
    "AuthorizeRemoteTxRequests",
    #"BlinkRepeat",
    "ClockAlignedDataInterval",
    "ConnectionTimeOut",
    "ConnectorPhaseRotation",
    "ConnectorPhaseRotationMaxLength",
    "GetConfigurationMaxKeys",
    "HeartbeatInterval",
    #"LightIntensity",
    "LocalAuthorizeOffline",
    "LocalPreAuthorize",
    #"MaxEnergyOnInvalidId",
    "MessageTimeout",
    "MeterValuesAlignedData",
    "MeterValuesAlignedDataMaxLength",
    "MeterValuesSampledData",
    "MeterValuesSampledDataMaxLength",
    "MeterValueSampleInterval",
    #"MinimumStatusDuration",
    "NumberOfConnectors",
    "ResetRetries",
    "StopTransactionOnEVSideDisconnect",
    "StopTransactionOnInvalidId",
    "StopTransactionMaxMeterValues",
    "StopTxnAlignedData",
    "StopTxnAlignedDataMaxLength",
    "StopTxnSampledData",
    "StopTxnSampledDataMaxLength",
    "SupportedFeatureProfiles",
    #"SupportedFeatureProfilesMaxLength",
    "TransactionMessageAttempts",
    "TransactionMessageRetryInterval",
    "UnlockConnectorOnEVSideDisconnect",
    "WebSocketPingInterval",

    # # LOCAL AUTH LIST MANAGEMENT PROFILE
    #"LocalAuthListEnabled",
    #"LocalAuthListMaxLength",
    #"SendLocalListMaxLength",

    # # RESERVATION PROFILE
    #"ReserveConnectorZeroSupported",

    # SMART CHARGING PROFILE
    "ChargeProfileMaxStackLevel",
    "ChargingScheduleAllowedChargingRateUnit",
    "ChargingScheduleMaxPeriods",
    "ConnectorSwitch3to1PhaseSupported",
    "MaxChargingProfilesInstalled",
]

"""
struct PlatformResponse {
    uint8_t seq_num;
    char tag_id_seen[22];
    uint8_t fixed_cable;
    uint8_t evse_state[NUM_CONNECTORS];
    uint32_t energy[NUM_CONNECTORS];
}  __attribute__((__packed__));

struct ConnectorMessage {
    uint8_t state;
    uint8_t last_sent_status;
    char tag_id[21];
    char parent_tag_id[21];
    uint8_t tag_status;
    time_t tag_expiry_date;
    uint32_t tag_deadline;
    uint32_t cable_deadline;
    int32_t txn_id;
    uint64_t transaction_confirmed_id;
    time_t transaction_start_time;
    uint32_t current_allowed;
    bool txn_with_invalid_id;
    bool unavailable_requested;
}  __attribute__((__packed__));

struct PlatformMessage {
    uint8_t seq_num = 0;
    char message[63] = "";
    uint32_t charge_current[NUM_CONNECTORS] = {0};
    uint8_t connector_locked = 0;

    uint8_t charge_point_state;
    uint8_t charge_point_last_sent_status;
    time_t next_profile_eval;

    uint8_t message_in_flight_type;
    int32_t message_in_flight_id;
    size_t message_in_flight_len;
    uint32_t message_timeout_deadline;
    uint32_t txn_msg_retry_deadline;
    uint8_t queue_depths[3];

    uint8_t config_key;
    char config_value[500];

    ConnectorMessage connector_messages[NUM_CONNECTORS];
}  __attribute__((__packed__));
"""

header_format = "<B" # uint8_t seq_num = 0;

connector_format = \
"B"     + \
"B"     + \
"21s"   + \
"21s"   + \
"B"     + \
"q"     + \
"I"     + \
"I"     + \
"i"     + \
"Q"     + \
"q"     + \
"I"     + \
"?"     + \
"?"


request_format = header_format + \
"63s"           + \
"{num_conn}I"   + \
"B"             + \
"B"             + \
"B"             + \
"q"             + \
"B"             + \
"Q"             + \
"Q"             + \
"I"             + \
"I"             + \
"B"             + \
"B"             + \
"B"             + \
"B"             + \
"500s"
request_format += connector_format * NUM_CONNECTORS
request_format = request_format.format(num_conn=NUM_CONNECTORS)

response_format = header_format + "22sB{num_conn}B{num_conn}I".format(num_conn=NUM_CONNECTORS)

request_len = struct.calcsize(request_format)
response_len = struct.calcsize(response_format)
