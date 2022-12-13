from queue import Queue, Empty
from threading import Thread

import argparse
import subprocess
import time
import os
import signal
import json
import sys
import re
import socket
import struct
from urllib.request import urlopen, Request
import datetime

from linux_platform_packet import *

octt_ip = None
octt_ws_port = None
octt_api_port = None
indent = 0
last_print_with_newline = True

def log(*args, **kwargs):
    global indent
    global last_print_with_newline
    if indent == 0 or not last_print_with_newline:
        print(*args, **kwargs)
    else:
        print(" " * (indent - 1), *args, **kwargs)

    last_print_with_newline = not "end" in kwargs

colors = {"off":"\x1b[00m",
          "blue":   "\x1b[34m",
          "cyan":   "\x1b[36m",
          "green":  "\x1b[32m",
          "red":    "\x1b[31m",
          "gray": "\x1b[90m"}

def red(s):
    return colors["red"]+s+colors["off"]

def green(s):
    return colors["green"]+s+colors["off"]

def gray(s):
    return colors['gray']+s+colors["off"]

def run_octt(task_queue: Queue[str]):
    process = subprocess.Popen(os.path.abspath("./octt/run_auto.sh"),
                               cwd=os.path.abspath("./octt"),
                               start_new_session=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    while True:
        try:
            x = task_queue.get(timeout=0.1)
        except Empty:
            try:
                process.communicate(timeout=0.1)
            except subprocess.TimeoutExpired:
                pass
            continue

        if x is None:
            try:
                os.killpg(os.getpgid(process.pid), signal.SIGTERM)
                process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                os.killpg(os.getpgid(process.pid), signal.SIGKILL)
            return
    pass


"""
(SUT Charging Station) In multiple testcases, StatusNotification(status=Charging) is missing in the expected
StatusNotification list. As a general rule: The Charging Station is allowed to send a
StatusNotification(status=Charging) when there is energy transfer. This happens after a transaction is started.
"""
energy_transfer_periods = []

EVSE_STATE_NOT_CONNECTED = 0
EVSE_STATE_PLUG_DETECTED = 1
EVSE_STATE_CONNECTED = 2
EVSE_STATE_READY_TO_CHARGE = 3
EVSE_STATE_CHARGING = 4
EVSE_STATE_FAULTED = 5

evse_state_auto = True

last_seen_seq_num = 0
next_seq_num = 0
next_tag_id = ""
evse_state = EVSE_STATE_NOT_CONNECTED
last_seen_connector_state = "IDLE"
last_charge_current = 0
def handle_ocpp_platform_request(data, addr, sock):
    global last_seen_seq_num
    global next_seq_num
    global next_tag_id
    global evse_state
    global evse_state_auto
    global last_seen_connector_state

    if len(data) < request_len:
        log("malformed request {} < {}".format(len(data), request_len))
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

    last_seen_connector_state = state
    last_charge_current = charge_current

    if (message != "POLL"):
        return

    b = struct.pack(response_format,
                next_seq_num,
                (bytearray([1]) + next_tag_id.encode("utf-8")) if next_tag_id else "".encode("utf-8"),
                evse_state,
                *([0] * (NUM_CONNECTORS - 1)),
                *([0] * NUM_CONNECTORS))

    if evse_state_auto:
        if evse_state == EVSE_STATE_CONNECTED and charge_current > 0:
            evse_state = EVSE_STATE_CHARGING
            energy_transfer_periods.append([datetime.datetime.now().astimezone().replace(microsecond=0).isoformat(), None])
        if evse_state == EVSE_STATE_CHARGING and charge_current == 0:
            evse_state = EVSE_STATE_CONNECTED
            energy_transfer_periods[-1][1] = datetime.datetime.now().astimezone().replace(microsecond=999999).isoformat()

    next_seq_num += 1
    next_seq_num %= 256

    sock.sendto(b, addr)
    next_tag_id = None

def run_ocpp(task_queue: Queue[str]):
    process = subprocess.Popen([os.path.abspath("./ocpp_linux"), "ws://{}:{}/ocpp".format(octt_ip, octt_ws_port)],
                               start_new_session=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)

    listen_addr = "127.0.0.2"

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((listen_addr, 34128))
    sock.setblocking(False)

    addr = None

    try:
        while True:
            try:
                data, addr = sock.recvfrom(request_len)
            except BlockingIOError:
                pass
            except Exception as e:
                log(e)
            else:
                try:
                    handle_ocpp_platform_request(data, addr, sock)
                except Exception as e:
                    log("!", e)

            try:
                x = task_queue.get(timeout=0.01)
            except Empty:
                try:
                    process.communicate(timeout=0.01)
                except subprocess.TimeoutExpired:
                    pass
            else:
                if x is None:
                    break
    finally:
        try:
            os.killpg(os.getpgid(process.pid), signal.SIGTERM)
            process.wait(timeout=5.0)
        except subprocess.TimeoutExpired:
            os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        return

def waiting_on_prompt():
    req = Request('http://{}:{}/ocpp/isWaitingOnPrompt'.format(octt_ip, octt_api_port))

    with urlopen(req, timeout=1) as resp:
        result = resp.read()

    waiting = json.loads(result)
    if isinstance(waiting, bool):
        return waiting

def get_prompt():
    req = Request('http://{}:{}/ocpp/getPrompt'.format(octt_ip, octt_api_port))

    with urlopen(req, timeout=1) as resp:
        result = resp.read()

    return result.decode('utf-8')

def advance_prompt(success: bool):
    if success:
        log(green("Advancing prompt"))
    else:
        log(red("Failing prompt"))

    req = Request('http://{}:{}/ocpp/autoExecPrompt'.format(octt_ip, octt_api_port))
    req.add_header('Content-Type', 'application/json')
    data = json.dumps({'continuePrompt': success, 'auto': 'api'}).encode('utf-8')
    with urlopen(req, timeout=5 * 60, data=data) as resp:
        resp.read()

    time.sleep(1)

exp_stn = re.compile(r"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}\+\d{2}:\d{2})?\s*-\s*ConnectorId\s*:\s*(\d+)\s*-\s*(\w+)\s*-\s*\n(\w+)\s*-\s*(N\/A)\s*-\s*(Yes|No)")
unexp_stn = re.compile(r"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}\+\d{2}:\d{2})?  - ConnectorId :(\d+)  -  (\w+)  -  \n(\w+) - (N\/A)")

def handle_expected_statusnotifications(p: str):
    log("Expected StatusNotification prompt received. Checking status notifications.", end=" ")
    expected_matches = (p.count("\n") - 4) / 3

    matches = exp_stn.findall(p)
    if len(matches) != expected_matches:
        raise Exception("Regex broken?")

    fail = False
    for match in matches:
        timestamp, connector_id, status, error_code, test_info, received = match
        if not received:
            log("\nExpected StatusNotification was not received! ", timestamp, connector_id, status, error_code, test_info, received)
            fail = True

    advance_prompt(not fail)

ignore_slow_intervals = False
def handle_expected_heartbeat(p: str):
    log("Expected heartbeat interval prompt received", end=" ")
    expected, actual = re.match(r"Expected Heartbeat Interval : (\d+)\nActual Heartbeat Interval : (\d+)", p).groups()
    log("Expected {} actual {}".format(expected, actual), end=" ")
    if ignore_slow_intervals and expected != actual:
        log(red("Ignoring slow heartbeat"), end=" ")
        advance_prompt(True)
    else:
        advance_prompt(expected == actual)

def handle_expected_meter_value_sample_interval(p: str):
    log("Expected meter value sample interval prompt received", end=" ")
    expected, actual = re.match(r"Expected MeterValue Sample Interval : (\d+)\nActual MeterValue Sample : (\d+)", p).groups()
    log("Expected {} actual {}".format(expected, actual), end=" ")
    if ignore_slow_intervals and expected != actual:
        log(red("Ignoring slow meter interval"), end=" ")
        advance_prompt(True)
    else:
        advance_prompt(expected == actual)

ignore_unexpected = False
def handle_unexpected_statusnotifications(p: str):
    expected_matches = (p.count("\n") - 4) / 3
    log("Unexpected StatusNotification prompt received. Checking status notifications.", end=" ")

    matches = unexp_stn.findall(p)
    if len(matches) != expected_matches:
        raise Exception("Regex broken?")

    for match in matches:
        timestamp, connector_id, status, error_code, test_info = match
        if status == "SUSPENDED_EVSE" and ignore_unexpected:
            log(red("Ignoring unexpected StatusNotification with status SUSPENDED_EVSE"), end=" ")
            continue

        if status == "CHARGING":
            skip = True
            for start, end in energy_transfer_periods:
                if datetime.datetime.fromisoformat(start) < datetime.datetime.fromisoformat(timestamp) \
                  and (end is None or datetime.datetime.fromisoformat(timestamp) < datetime.datetime.fromisoformat(end)):
                    log("Working around missing charging status notification", end=" ")
                    break
            else:
                skip = False
            if skip:
                continue

        log(f"\nUnexpected StatusNotification! {timestamp=} {connector_id=} {status=} {error_code=} {test_info=}")
        advance_prompt(False)
        return

    advance_prompt(True)

ocpp_queue = Queue()
ocpp_thread = None

tc_069_workaround_counter = 0

def handle_prompt(p: str, test_case: str):
    global ocpp_thread

    global next_tag_id
    global evse_state

    global tc_069_workaround_counter

    # Every test
    if p == "Ensure that charge point is setup for this test.":
        log("Setup prompt received.", end=" ")
        if ocpp_thread is None:
            log("Starting OCPP.", end=" ")
            ocpp_thread = Thread(target=run_ocpp, args=[ocpp_queue])
            ocpp_thread.start()
            time.sleep(2)
        else:
            log("OCPP already running", end=" ")
        advance_prompt(True)
    # Every test
    elif p.startswith("Expected StatusNotifications\n"):
        handle_expected_statusnotifications(p)
    # TC_CP_V16_001
    elif p == "Power cycle the charge point and then resume the test case by clicking 'Yes'.":
        log("Power cycle prompt received.", end=" ")
        if ocpp_thread is None:
            log("Can't power cycle if OCPP is not running")
            advance_prompt(False)
            raise Exception("Can't power cycle if OCPP is not running")

        advance_prompt(True)
        log("Restarting OCPP")
        ocpp_queue.put(None)
        ocpp_thread.join()
        ocpp_thread = Thread(target=run_ocpp, args=[ocpp_queue])
        ocpp_thread.start()
    # TC_CP_V16_001
    elif p.startswith("Expected Heartbeat Interval :"):
        handle_expected_heartbeat(p)
    # TC_CP_V16_003
    elif p == "First resume the test case by clicking 'Yes' and plugin cable.":
        log("Plug in cable prompt received. Plugging in cable", end=" ")
        advance_prompt(True)
        evse_state = EVSE_STATE_CONNECTED
    # TC_CP_V16_003
    elif p == "First resume the test case by clicking 'Yes' and present valid identification.":
        if test_case == "TC_CP_V16_005_2":
            log('TC_CP_V16_005_2: Work-around for wrong "Present valid tag" prompt activated.', end=" ")
            advance_prompt(True)
            return

        log("Present valid identification prompt received. Presenting tag", end=" ")
        advance_prompt(True)
        next_tag_id = "Valid"
    # TC_CP_V16_003
    elif p.startswith("Expected MeterValue Sample Interval :"):
        handle_expected_meter_value_sample_interval(p)
    # TC_CP_V16_003
    elif p == "EV driver stops the transaction":
        log("Present same identification prompt received. Presenting tag", end=" ")
        advance_prompt(True)
        next_tag_id = "Valid"
    elif p == "First resume the test case by clicking 'Yes' and Unplug cable.":
        log("Unplug cable prompt received. Unplugging cable", end=" ")
        advance_prompt(True)
        evse_state = EVSE_STATE_NOT_CONNECTED
    elif p == "Is Charge Point connector status 'Available' ?" or \
         p == "Is connector status 'Available' at central system?": # TC_CP_V16_005_1; This is probably a wrong prompt: The central is the test tool. In the test context it makes sense to check whether the connector status is Available _at the charge point_
        log("Checking if connector is available", end=" ")
        advance_prompt(last_seen_connector_state == "IDLE")
    elif p.startswith("UnExpected StatusNotifications"):
        handle_unexpected_statusnotifications(p)
    # TC_CP_V16_005_1
    elif p == "First resume the test case by clicking 'Yes'and unplug at EV side.":
        log("Unplug at EV side prompt received. Unplugging", end=" ")
        advance_prompt(True)
        evse_state = EVSE_STATE_PLUG_DETECTED
    # TC_CP_V16_012
    elif p == "Sending Remote Stop Transaction" and test_case == "TC_CP_V16_012":
        # Redundant popup "Sending Remote Stop Transaction". Just click yes.
        log("TC_CP_V16_012: Work-aroung for redundant prompt.", end=" ")
        advance_prompt(True)
    elif p == "First resume the test case by clicking 'Yes' and then Stop charging" and test_case == "TC_CP_V16_012":
        # Incorrect popup "First resume the test case by clicking 'Yes' and then Stop charging". Press yes ASAP, otherwise the StopTransaction.req validation might be too late.
        log("TC_CP_V16_012: Work-aroung for incorrect prompt.", end=" ")
        advance_prompt(True)
    elif p == "First resume the test case by clicking 'Yes' and present invalid identification.":
        log("Present invalid identification prompt received. Presenting tag", end=" ")
        advance_prompt(True)
        next_tag_id = "Invalid"
    #TC_CP_V16_068
    elif p == "EV driver authorizes/swipes a card resume the test case by clicking 'Yes'." \
      or p == "EV driver authorizes/swipes with a same card used to start transaction & resume the test case by clicking 'Yes'.":
        if test_case == "TC_CP_V16_069":
            tc_069_workaround_counter += 1
        if tc_069_workaround_counter == 2:
            log("Working around wrong prompt TC_CP_V16_069", end=" ")
            advance_prompt(True)
            next_tag_id = "RFID2"
            return

        log("Present valid identification prompt received. Presenting tag", end=" ")
        advance_prompt(True)
        next_tag_id = "RFID1"
    #TC_CP_V16_068
    elif p == "EV driver authorizes/swipes with a different card & resume the test case by clicking 'Yes'." and test_case == "TC_CP_V16_068":
        log("Present another valid identification prompt received. Working around bug in TC_CP_V16_068", end=" ")
        advance_prompt(True)
        #next_tag_id = "RFID2"
    else:
        log("Unknown prompt: {}".format(p))

def run_test(testcases, test_case: str):
    log("Executing test {}: {}".format(test_case, testcases[test_case]))
    global indent
    indent = 4

    result_queue = Queue()
    def inner(test_case: str):
        req = Request('http://{}:{}/ocpp/autoExecTestcase'.format(octt_ip, octt_api_port))
        req.add_header('Content-Type', 'application/json')
        data = json.dumps({'testCaseID': test_case, 'executionType': 'start'}).encode('utf-8')
        with urlopen(req, timeout=5 * 60, data=data) as resp:
            result = resp.read().decode('utf-8')
        result_queue.put(result)

    test_thread = Thread(target=inner, args=[test_case])
    test_thread.start()

    while True:
        try:
            result = json.loads(result_queue.get_nowait())["testCase_Result"]

            if test_case == "TC_CP_V16_023":
                log("TC_CP_V16_023 work-around: Waiting for (unauthorized!) start of transaction.", end=" ")
                time.sleep(5)
                if last_charge_current != 0:
                    print(red("Transaction was started"))
                    result = "FAIL"
                else:
                    print(green("No unauthorized transaction started"))


            indent = 0
            log("Test {} result".format(test_case), red(result) if result != "PASS" else green(result))
            if result != "PASS":
                raise Exception("Test did not pass")
            return
        except Empty:
            pass
        time.sleep(1)
        if waiting_on_prompt():
            handle_prompt(get_prompt(), test_case)

def get_testcases():
    req = Request('http://{}:{}/ocpp/getTestcases'.format(octt_ip, octt_api_port))
    with urlopen(req, timeout=3) as resp:
        result = json.loads(resp.read())
    return {x['name']: x['desc'] for x in result["testcases"]}

working_testcases = [
    "TC_CP_V16_001",
    "TC_CP_V16_002",
    "TC_CP_V16_003",
    "TC_CP_V16_004_1",
    "TC_CP_V16_004_2",
    "TC_CP_V16_005_1",
    "TC_CP_V16_005_2",
    "TC_CP_V16_010",
    "TC_CP_V16_011_2",
    "TC_CP_V16_017_1",
    "TC_CP_V16_018_1",
    "TC_CP_V16_021",
    "TC_CP_V16_023",
    "TC_CP_V16_026",
    "TC_CP_V16_027",
    "TC_CP_V16_028",
    "TC_CP_V16_031",
    "TC_CP_V16_040_1"
    "TC_CP_V16_040_2",
    "TC_CP_V16_056",
    "TC_CP_V16_057",
    "TC_CP_V16_058_1",
    "TC_CP_V16_058_2",
    "TC_CP_V16_059",
    "TC_CP_V16_060",
    "TC_CP_V16_062",
    "TC_CP_V16_067",
    "TC_CP_V16_068",
    "TC_CP_V16_070",
    "TC_CP_V16_071",
    "TC_CP_V16_072",
    "TC_CP_V16_082"
]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('octt_ip', help="OCTT ip for API calls.")
    parser.add_argument('--api-port', help="OCTT HTTP API port. Default 63341", default=63341)
    parser.add_argument('--ws-port', help="OCTT websocket port. Default 8080", default=8080)
    parser.add_argument('test_cases', nargs='*')
    # No short form for you: It has to hurt to type this out.
    parser.add_argument('--ignore-unexpected-suspended-status-notifications', action='store_true')
    parser.add_argument('--ignore-slow-intervals', action='store_true')
    parser.add_argument('-p', '--pause-before-test', action='store_true')

    args = parser.parse_args()

    global octt_ip
    global octt_ws_port
    global octt_api_port

    octt_ip = args.octt_ip
    octt_api_port = args.api_port
    octt_ws_port = args.ws_port

    if args.ignore_unexpected_suspended_status_notifications:
        global ignore_unexpected
        ignore_unexpected = True

    if args.ignore_slow_intervals:
        global ignore_slow_intervals
        ignore_slow_intervals = True

    if args.pause_before_test:
        input("Start wireshark now.")

    octt_queue = Queue()

    octt_thread = Thread(target=run_octt, args=[octt_queue])
    octt_thread.start()

    try:
        log("Waiting for OCTT to start", end='')

        while True:
            time.sleep(1)
            try:
                waiting_on_prompt()
                break
            except:
                pass
            log('.', end='')
        log('\nOCTT started')

        test_case_descs = get_testcases()

        if len(args.test_cases) > 0:
            testcases = args.test_cases
        else:
            testcases = working_testcases

        for testcase in testcases:
            run_test(test_case_descs, "TC_CP_V16_000_RESET")

            if args.pause_before_test:
                input("Press enter to start test")

            run_test(test_case_descs, testcase)

    finally:
        global ocpp_thread
        if ocpp_thread is not None:
            ocpp_queue.put(None)
            ocpp_thread.join()

        octt_queue.put(None)
        octt_thread.join()

if __name__ == "__main__":
    main()


"""
! = errata
# = missing/incorrect popup (in errata)

working
-------
TC_CP_V16_000_RESET Revert Charge Point to basic reset state
TC_CP_V16_000_STOPTX General Script - Charge Point - Stop charging session
TC_CP_V16_001 Cold boot charge point
TC_CP_V16_002 Cold boot charge point-pending
TC_CP_V16_003 Regular charging session - plugin first
TC_CP_V16_004_1 Regular charging session - identification first
TC_CP_V16_004_2 Regular charging session - identification first - ConnectionTimeOut
TC_CP_V16_005_1 EV side disconnected
## TC_CP_V16_005_2 EV side disconnected
# TC_CP_V16_010 Remote start charging session - cable plugged in first
TC_CP_V16_011_2 Remote Start Charging Session - Time Out
TC_CP_V16_017_1 Unlock connector no charging session running(Not fixed cable)
TC_CP_V16_018_1 Unlock connector with charging session
TC_CP_V16_021 Change/Set configuration
! TC_CP_V16_023 Start charging session - authorize invalid / blocked / expired
TC_CP_V16_026 Remote start charging session - rejected
TC_CP_V16_027 Remote start transaction - connector id shall not be 0
TC_CP_V16_028 Remote stop transaction - rejected
TC_CP_V16_031 Unlock connector - unknown connector
TC_CP_V16_056 Central Smart Charging - TxDefaultProfile
TC_CP_V16_057 Central Smart Charging - TxProfile
TC_CP_V16_058_1 Central Smart Charging - No ongoing transaction
TC_CP_V16_058_2 Central Smart Charging - Wrong transactionId
TC_CP_V16_059 Remote Start Transaction with Charging Profile
TC_CP_V16_060 Remote Start Transaction with Charging Profile - Rejected
TC_CP_V16_067 Clear Charging Profile
! TC_CP_V16_068 Stop transaction-IdTag stop transaction matches IdTag start transaction
TC_CP_V16_070 Sampled Meter Values
TC_CP_V16_071 Clock-aligned Meter Values
TC_CP_V16_072 Stacking Charging Profiles
TC_CP_V16_082 The Central System sets a default schedule for a currently ongoing transaction.

working - missing verification
------------------------------
TC_CP_V16_040_1 Configuration keys
! TC_CP_V16_040_2 Configuration keys
    - both tests don't check for the ChangeConfiguration.conf result (NotSupported/Rejected)

TC_CP_V16_062 Data Transfer to a Charge Point
    - We reject all data transfers and this is accepted.

not working yet
---------------
# TC_CP_V16_011_1 Remote Start Charging Session – Remote Start First
    This seems to be a timing issue with the test tool: The tool waits forever for a StopTxn.req however this req is sent.
## TC_CP_V16_012 Remote stop charging session
    This has the same timing issue, but with a incorrect prompt that makes the test tool miss our StopTxn.req every time.

TC_CP_V16_013 Hard reset without transaction
TC_CP_V16_014 Soft reset without transaction
!! TC_CP_V16_015 Hard reset with transaction
TC_CP_V16_016 Soft reset with transaction
    - Maybe an issue with the way run_octt is starting ocpp_linux? It does not come back after a hard/soft reset

TC_CP_V16_017_2 Unlock connector no charging session running(Fixed cable)
    - We have to report Not supported if we receive a UnlockConnector.req and have a fixed cable
TC_CP_V16_018_2 Unlock connector with charging session
    - We have to report Not supported if we receive a UnlockConnector.req and have a fixed cable

TC_CP_V16_019 Retrieve all configuration keys
    - Test tool hangs forever?

TC_CP_V16_030 Unlock connector - unlock failure
    - No way to simulate this yet

TC_CP_V16_035 Idle charge point
!! TC_CP_V16_036 Connection loss during transaction
TC_CP_V16_037_1 Offline Start Transaction
# TC_CP_V16_037_2 Offline start transaction
### TC_CP_V16_037_3 Offline start transaction
TC_CP_V16_038 Offline stop transaction
TC_CP_V16_039 Offline transaction
    - run_octt currently has no way of disconnecting the web socket connection

TC_CP_V16_066 Get Composite Schedule
    - OCTT randomly closes the web socket connection?!?

# TC_CP_V16_069 Stop transaction-ParentIdTag stop transaction matches ParentIdTag start transaction
    - We don't track the tag ID in flight correctly

TC_CP_V16_032_1 Power failure boot charging point - configured to stop transaction(s)  Stop all transactions before going down
# TC_CP_V16_032_2 Power failure boot charging point-configured to stop transaction(s)
TC_CP_V16_034 Power failure with unavailable status
    - No way to simulate power failure yet

prerequisites not supported
---------------------------
TC_CP_V16_005_3 EV side disconnected
    This requires StopTransactionOnEVSideDisconnect to be settable to false. We don't allow this.
TC_CP_V16_006 One reader for multiple connectors
    We don't support more than one connector yet.
## TC_CP_V16_007 Regular start charging session - cached id
    Authorization cache not implemented yet
### TC_CP_V16_008 Regular start charging session - id in authorization list
    Authorization cache not implemented yet
TC_CP_V16_024 Start Charging Session Lock Failure
    No platform API to communicate lock failure yet
TC_CP_V16_041 Fault behavior
    No platform API to fault charge point yet

TC_CP_V16_042_1 Get Local List Version,Non-Happy case
TC_CP_V16_042_2 Get Local List Version,Local Authorization List Empty
TC_CP_V16_043 SendLocalAuthorizationList-UpdatedType=Differential & Full
TC_CP_V16_043_1 SendLocalAuthorizationList-UpdatedStatus=NotSupported
TC_CP_V16_043_2 SendLocalAuthorizationList-UpdatedStatus=VersionMisMatch
TC_CP_V16_043_3 SendLocalAuthorizationList-UpdatedStatus=FAILED
TC_CP_V16_044_1 Firmware Update -Download and Install
TC_CP_V16_044_2 Firmware Update -Download-failed ;Status Notification
TC_CP_V16_044_3 Firmware Update -Installation-Failed;Status Notification
TC_CP_V16_045_1 Get Diagonstics of charge point
TC_CP_V16_045_2 Get Diagnostics of charge point;Upload failed
TC_CP_V16_046_1 Reservation of a Connector - Local start transaction
! TC_CP_V16_046_2 Reservation of a connector - Remote start transaction
# TC_CP_V16_047 Reservation of a Connector - Expire
TC_CP_V16_048_1 Reservation of a Connector -Faulted
TC_CP_V16_048_2 Reservation of a Connector -Occupied
TC_CP_V16_048_3 Reservation of a Connector-Unavailable
TC_CP_V16_048_4 Reservation of a Connector- Rejected
TC_CP_V16_049 Reservation of a Charge Point- Transaction
TC_CP_V16_050_1 Reservation of a Charge Point- Faulted
!! TC_CP_V16_050_2 Reservation of a Charge Point- Occupied
TC_CP_V16_050_3 Reservation of a Charge Point- Unavailable
TC_CP_V16_050_4 Reservation of a Charge Point- Rejected
! TC_CP_V16_051 Cancel Reservation-Accepted
TC_CP_V16_052 Cancel Reservation - Rejected
TC_CP_V16_053 Use a reserved Connector with parentIdTag
TC_CP_V16_054 Trigger Message
TC_CP_V16_055 Trigger Message - Rejected
TC_CP_V16_061 Clear Authorization Data in Authorization Cache
TC_CP_V16_064 Data Transfer to a Central System (Listener)
    - Unsupported feature profiles

TC_CP_V16_073 Update Charge Point Password for HTTP Basic Authentication
TC_CP_V16_074 Update Charge Point Certificate by request of Central System
TC_CP_V16_075 Install a certificate on the Charge Point
TC_CP_V16_076 Delete a specific certificate from the Charge Point
TC_CP_V16_077 Invalid ChargePointCertificate Security Event
TC_CP_V16_078 Invalid CentralSystemCertificate Security Event
TC_CP_V16_079 Get Security Log
! TC_CP_V16_080 Secure Firmware Update
! TC_CP_V16_081 Secure Firmware Update - Invalid Signature
    - Security whitepaper not implemented yet

not tested yet
--------------

"""
