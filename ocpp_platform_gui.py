import socket
import struct
import sys
import time

"""
struct PlatformMessage {
    uint8_t seq_num = 0;
    char message[63] = "";
    uint32_t charge_current[8];
    uint8_t connector_locked;
}  __attribute__((__packed__));

struct PlatformResponse {
    uint8_t seq_num;
    char tag_id_seen[22];
    uint8_t evse_state[8];
    uint32_t energy[8];
}  __attribute__((__packed__));
"""

header_format = "<B"
request_format = header_format + "63s8IB"
response_format = header_format + "22s8B8I"

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
from PyQt5.QtCore import QTimer

app = QApplication([])
window = QWidget()
layout = QFormLayout()
layout.addRow(QLabel("Request"))

req_seq_num = QLabel("no packet received yet")
layout.addRow("Sequence number", req_seq_num)

req_message = QLabel("no packet received yet")
layout.addRow("Message", req_message)

req_charge_current = QLabel("no packet received yet")
layout.addRow("Charge current", req_charge_current)

req_locked = QLabel("no packet received yet")
layout.addRow("Cable locked", req_locked)

layout.addRow(QLabel("Response"))

resp_seq_num = QLabel("no packet sent yet")
layout.addRow("Sequence number", resp_seq_num)

resp_block_seq_num = QCheckBox("Block sequence number")
layout.addRow("", resp_block_seq_num)

resp_tag_at_connector = QComboBox()
resp_tag_at_connector.addItem("1")
resp_tag_at_connector.addItem("2")
resp_tag_at_connector.addItem("3")
resp_tag_at_connector.addItem("4")
resp_tag_at_connector.addItem("5")
resp_tag_at_connector.addItem("6")
resp_tag_at_connector.addItem("7")
resp_tag_at_connector.addItem("8")
resp_tag_at_connector.addItem("9")
layout.addRow("Show Tag at connector", resp_tag_at_connector)

resp_tag_id = QLineEdit("")
resp_tag_id.setPlaceholderText("Type Tag ID to show")
resp_tag_id.setMaxLength(20)
layout.addRow("Show Tag ID", resp_tag_id)

resp_send_tag_id = QCheckBox("Show Tag")
layout.addRow("", resp_send_tag_id)

resp_tag_id.returnPressed.connect(lambda: resp_send_tag_id.setChecked(True))

resp_evse_state = QComboBox()
resp_evse_state.addItem("0: NotConnected")
resp_evse_state.addItem("1: Connected")
resp_evse_state.addItem("2: ReadyToCharge")
resp_evse_state.addItem("3: Charging")
resp_evse_state.addItem("4: Faulte")
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
    if len(data) != request_len:
        return

    charge_current = []
    seq_num, message, *charge_current, connector_locked = struct.unpack(request_format, data)
    if seq_num == last_seen_seq_num:
        return
    last_seen_seq_num = seq_num

    message = message.decode("utf-8").replace("\0", "")

    req_charge_current.setText(", ".join(["{:3.3f} A".format(cc / 1000.0) for cc in charge_current]))
    req_locked.setText(", ".join("Locked" if (connector_locked & (1 << i)) else "Unlocked" for i in range(8)))

    req_seq_num.setText(str(seq_num))
    if (message != "POLL"):
        req_message.setText(message)
        return

    b = struct.pack(response_format,
                    next_seq_num,
                    (bytearray([resp_tag_at_connector.currentIndex() + 1]) + resp_tag_id.text().encode("utf-8")) if resp_send_tag_id.isChecked() else "".encode("utf-8"),
                    resp_evse_state.currentIndex(),
                    *([0] * 7),
                    *([0] * 8))

    if not resp_block_seq_num.isChecked():
        next_seq_num += 1
        next_seq_num %= 256

    sock.sendto(b, addr)
    resp_send_tag_id.setChecked(False)

recv_timer = QTimer()
recv_timer.timeout.connect(receive)
recv_timer.start(100)

# send_timer = QTimer()
# send_timer.timeout.connect(send)
# send_timer.start(1000)

window.setLayout(layout)
window.show()
app.exec_()
