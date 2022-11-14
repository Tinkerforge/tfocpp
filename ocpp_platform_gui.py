import socket
import struct
import sys
import time

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
    time_t transaction_confirmed_timestamp;
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
"q"     + \
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
"i"             + \
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

response_format = header_format + "22s{num_conn}B{num_conn}I".format(num_conn=NUM_CONNECTORS)

request_len = struct.calcsize(request_format)
response_len = struct.calcsize(response_format)
print(request_len)
print(response_len)

listen_addr = "127.0.0.2" #sys.argv[1]

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((listen_addr, 34128))
sock.setblocking(False)

addr = None

from PyQt5.QtWidgets import *
from PyQt5.QtCore import QTimer, Qt

app = QApplication([])
window = QWidget()
layout = QFormLayout()
layout.addRow(QLabel("Request"))

req_seq_num = QLabel("no packet received yet")
layout.addRow("Sequence number", req_seq_num)

req_message = QLabel("no packet received yet")
layout.addRow("Message", req_message)

lbl = QLabel("--------------------------------------------------Charge point--------------------------------------------------")
lbl.setAlignment(Qt.AlignCenter)
layout.addRow(lbl)

req_charge_current = QLabel("no packet received yet")
layout.addRow("Charge current", req_charge_current)

req_locked = QLabel("no packet received yet")
layout.addRow("Cable locked", req_locked)

req_charge_point_state = QLabel("no packet received yet")
layout.addRow("State", req_charge_point_state)

req_charge_point_last_sent_status = QLabel("no packet received yet")
layout.addRow("Last sent status", req_charge_point_last_sent_status)

req_next_profile_eval = QLabel("no packet received yet")
layout.addRow("Next profile eval", req_next_profile_eval)

req_message_in_flight_type = QLabel("no packet received yet")
layout.addRow("Message in flight type", req_message_in_flight_type)

req_message_in_flight_id = QLabel("no packet received yet")
layout.addRow("MsgIF ID", req_message_in_flight_id)

req_message_in_flight_len = QLabel("no packet received yet")
layout.addRow("MsgIF len", req_message_in_flight_len)

req_message_timeout_deadline = QLabel("no packet received yet")
layout.addRow("MsgIf timeout", req_message_timeout_deadline)

req_txn_msg_retry_deadline = QLabel("no packet received yet")
layout.addRow("MsgId timeout", req_txn_msg_retry_deadline)

req_message_queue_depth = QLabel("no packet received yet")
layout.addRow("Message queue depth", req_message_queue_depth)

req_status_notification_queue_depth = QLabel("no packet received yet")
layout.addRow("StatusNotification queue depth", req_status_notification_queue_depth)

req_transaction_message_queue_depth = QLabel("no packet received yet")
layout.addRow("Transaction message queue depth", req_transaction_message_queue_depth)

lbl = QLabel("--------------------------------------------------Connector--------------------------------------------------")
lbl.setAlignment(Qt.AlignCenter)
layout.addRow(lbl)

req_state = QLabel("no packet received yet")
layout.addRow("Connector state", req_state)
req_last_sent_status = QLabel("no packet received yet")
layout.addRow("Last sent status", req_last_sent_status)
req_tag_id = QLabel("no packet received yet")
layout.addRow("Tag ID", req_tag_id)
req_parent_tag_id = QLabel("no packet received yet")
layout.addRow("Parent tag ID", req_parent_tag_id)
req_tag_status = QLabel("no packet received yet")
layout.addRow("Tag status", req_tag_status)
req_tag_expiry_date = QLabel("no packet received yet")
layout.addRow("Tag expiry date", req_tag_expiry_date)
req_tag_deadline = QLabel("no packet received yet")
layout.addRow("Tag deadline", req_tag_deadline)
req_cable_deadline = QLabel("no packet received yet")
layout.addRow("Cable deadline", req_cable_deadline)
req_txn_id = QLabel("no packet received yet")
layout.addRow("Transaction ID", req_txn_id)
req_transaction_confirmed_timestamp = QLabel("no packet received yet")
layout.addRow("Transaction confirmed timestamp", req_transaction_confirmed_timestamp)
req_transaction_start_time = QLabel("no packet received yet")
layout.addRow("Transaction start time", req_transaction_start_time)
req_current_allowed = QLabel("no packet received yet")
layout.addRow("Current allowed", req_current_allowed)
req_txn_with_invalid_id = QLabel("no packet received yet")
layout.addRow("Txn with invalid id", req_txn_with_invalid_id)
req_unavailable_requested = QLabel("no packet received yet")
layout.addRow("Unavailable requested", req_unavailable_requested)

lbl = QLabel("--------------------------------------------------Configuration--------------------------------------------------")
lbl.setAlignment(Qt.AlignCenter)
layout.addRow(lbl)

req_config = QListWidget()
layout.addRow("Configuration", req_config)

layout.addRow(QLabel("Response"))

resp_seq_num = QLabel("no packet sent yet")
layout.addRow("Sequence number", resp_seq_num)

# resp_block_seq_num = QCheckBox("Block sequence number")
# layout.addRow("", resp_block_seq_num)

# resp_tag_at_connector = QComboBox()
# resp_tag_at_connector.addItem("1")
# resp_tag_at_connector.addItem("2")
# resp_tag_at_connector.addItem("3")
# resp_tag_at_connector.addItem("4")
# resp_tag_at_connector.addItem("5")
# resp_tag_at_connector.addItem("6")
# resp_tag_at_connector.addItem("7")
# resp_tag_at_connector.addItem("8")
# resp_tag_at_connector.addItem("9")
# layout.addRow("Show Tag at connector", resp_tag_at_connector)

resp_tag_id = QLineEdit("")
resp_tag_id.setPlaceholderText("Type Tag ID to show")
resp_tag_id.setMaxLength(20)
layout.addRow("Show Tag ID", resp_tag_id)

resp_send_tag_id = QCheckBox("Show Tag")
layout.addRow("", resp_send_tag_id)

resp_tag_id.returnPressed.connect(lambda: resp_send_tag_id.setChecked(True))

resp_evse_state = QComboBox()
resp_evse_state.addItem("0: NotConnected")
resp_evse_state.addItem("1: PlugDetected")
resp_evse_state.addItem("2: Connected")
resp_evse_state.addItem("3: ReadyToCharge")
resp_evse_state.addItem("4: Charging")
resp_evse_state.addItem("5: Faulte")
layout.addRow("EVSE State", resp_evse_state)
"""
    resp_charger_state = QComboBox()
    resp_charger_state.addItem("0: Not connected")
    resp_charger_state.addItem("1: Waiting for release")
    resp_charger_state.addItem("2: Ready")
    resp_charger_state.addItem("3: Charging")
    resp_charger_state.addItem("4: Error")
    layout.addRow("Vehicle State", resp_charger_state)

    resp_error_state = QSpinBox()
    resp_error_state.setMinimum(0)
    resp_error_state.setMaximum(4)
    layout.addRow("Error State", resp_error_state)

    resp_uptime = QLabel("no packet sent yet")
    layout.addRow("Uptime", resp_uptime)

    resp_block_uptime = QCheckBox("Block uptime")
    layout.addRow("", resp_block_uptime)

    resp_charging_time = QLabel("no packet sent yet")
    layout.addRow("Charging time", resp_charging_time)

    resp_allowed_charging_current = QSpinBox()
    resp_allowed_charging_current.setMinimum(0)
    resp_allowed_charging_current.setMaximum(32)
    resp_allowed_charging_current.setSuffix(" A")
    layout.addRow("Allowed charging current", resp_allowed_charging_current)

    resp_supported_current = QSpinBox()
    resp_supported_current.setMinimum(6)
    resp_supported_current.setMaximum(32)
    resp_supported_current.setSuffix(" A")
    layout.addRow("Supported current", resp_supported_current)

    resp_managed = QCheckBox("")
    resp_managed.setChecked(True)
    layout.addRow("Managed", resp_managed)
"""

next_seq_num = 0
protocol_version = 3
start = time.time()
charging_time_start = 0

last_seen_seq_num = -1
def receive():
    global addr
    global charging_time_start
    global next_seq_num
    global last_seen_seq_num
    global addr

    try:
        data, addr = sock.recvfrom(request_len)
    except BlockingIOError:
        return
    if len(data) < request_len:
        print("malformed request {} < {}".format(len(data), request_len))
        return

    charge_current = []
    seq_num, \
    message, \
    charge_current, \
    connector_locked, \
    charge_point_state, \
    charge_point_last_sent_status, \
    next_profile_eval, \
    message_in_flight_type, \
    message_in_flight_id, \
    message_in_flight_len, \
    message_timeout_deadline, \
    txn_msg_retry_deadline, \
    message_queue_depth, \
    status_notification_queue_depth, \
    transaction_message_queue_depth, \
    config_key, \
    config_value, \
    state, \
    last_sent_status, \
    tag_id, \
    parent_tag_id, \
    tag_status, \
    tag_expiry_date, \
    tag_deadline, \
    cable_deadline, \
    txn_id, \
    transaction_confirmed_timestamp, \
    transaction_start_time, \
    current_allowed, \
    txn_with_invalid_id, \
    unavailable_requested \
        = struct.unpack(request_format, data)
    if seq_num == last_seen_seq_num:
        return
    last_seen_seq_num = seq_num

    message = message.decode("utf-8").replace("\0", "")
    config_value = config_value.decode("utf-8").replace("\0", "")
    tag_id = tag_id.decode("utf-8").replace("\0", "")
    parent_tag_id = parent_tag_id.decode("utf-8").replace("\0", "")

    charge_point_state = charge_point_state_strings[charge_point_state]
    charge_point_last_sent_status = status_notification_status_strings[charge_point_last_sent_status]
    message_in_flight_type = message_in_flight_type_strings[message_in_flight_type]
    config_key = config_key_strings[config_key]
    state = connector_state_strings[state]
    last_sent_status = status_notification_status_strings[last_sent_status]
    tag_status = tag_status_strings[tag_status]

    req_charge_current.setText("{:3.3f} A".format(charge_current / 1000.0))
    req_locked.setText(", ".join("Locked" if (connector_locked & (1 << i)) else "Unlocked" for i in range(1)))

    req_charge_point_state.setText(str(charge_point_state))
    req_charge_point_last_sent_status.setText(str(charge_point_last_sent_status))
    req_next_profile_eval.setText(str(next_profile_eval))
    req_message_in_flight_type.setText(str(message_in_flight_type))
    req_message_in_flight_id.setText(str(message_in_flight_id))
    req_message_in_flight_len.setText(str(message_in_flight_len))
    req_message_timeout_deadline.setText(str(message_timeout_deadline))
    req_txn_msg_retry_deadline.setText(str(txn_msg_retry_deadline))
    req_message_queue_depth.setText(str(message_queue_depth))
    req_status_notification_queue_depth.setText(str(status_notification_queue_depth))
    req_transaction_message_queue_depth.setText(str(transaction_message_queue_depth))

    req_state.setText(str(state))
    req_last_sent_status.setText(str(last_sent_status))
    req_tag_id.setText(str(tag_id))
    req_parent_tag_id.setText(str(parent_tag_id))
    req_tag_status.setText(str(tag_status))
    req_tag_expiry_date.setText(str(tag_expiry_date))
    req_tag_deadline.setText(str(tag_deadline))
    req_cable_deadline.setText(str(cable_deadline))
    req_txn_id.setText(str(txn_id))
    req_transaction_confirmed_timestamp.setText(str(transaction_confirmed_timestamp))
    req_transaction_start_time.setText(str(transaction_start_time))
    req_current_allowed.setText(str(current_allowed))
    req_txn_with_invalid_id.setText(str(txn_with_invalid_id))
    req_unavailable_requested.setText(str(unavailable_requested))

    for i in range(req_config.count()):
        if req_config.item(i).text().startswith("{}: ".format(config_key)):
            req_config.item(i).setText("{}: {}".format(config_key, config_value))
            break
    else:
        req_config.addItem("{}: {}".format(config_key, config_value))

    req_seq_num.setText(str(seq_num))
    if (message != "POLL"):
        req_message.setText(message)
        return

    b = struct.pack(response_format,
                    next_seq_num,
                    (bytearray([1]) + resp_tag_id.text().encode("utf-8")) if resp_send_tag_id.isChecked() else "".encode("utf-8"),
                    resp_evse_state.currentIndex(),
                    *([0] * (NUM_CONNECTORS - 1)),
                    *([0] * NUM_CONNECTORS))

    resp_seq_num.setText(str(next_seq_num))

    #if not resp_block_seq_num.isChecked():
    next_seq_num += 1
    next_seq_num %= 256

    sock.sendto(b, addr)
    resp_send_tag_id.setChecked(False)

recv_timer = QTimer()
recv_timer.timeout.connect(receive)
recv_timer.start(10)

# send_timer = QTimer()
# send_timer.timeout.connect(send)
# send_timer.start(1000)

window.setLayout(layout)
window.show()
app.exec_()
