import ctypes
import os
import queue
import sys
import time
import threading

thread_id = None
work_queue = queue.Queue()
timescale = 1
time_start = None

@ctypes.CFUNCTYPE(ctypes.c_uint32)
def platform_now_ms():
    return int((time.time() - time_start) * timescale * 1000)

last_time_set_at = 0
last_time_set = 0

@ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_long)
def platform_set_system_time(ctx, t):
    global last_time_set
    global last_time_set_at
    last_time_set = t
    last_time_set_at = time.time()

@ctypes.CFUNCTYPE(ctypes.c_long, ctypes.c_void_p)
def platform_get_system_time(ctx):
    return int((time.time() - last_time_set_at) * timescale + last_time_set)

@ctypes.CFUNCTYPE(None, ctypes.c_char_p)
def platform_printfln(ptr):
    #sys.stdout.buffer.write(ptr + os.linesep.encode("ascii"))
    #sys.stdout.flush()
    print(ptr.decode("utf-8"))

tag_seen_cb = None
tag_seen_cb_user_data = None

@ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.CFUNCTYPE(None, ctypes.c_int32, ctypes.c_char_p, ctypes.c_void_p), ctypes.c_void_p)
def platform_register_tag_seen_callback(ctx, cb, user_data):
    global tag_seen_cb
    global tag_seen_cb_user_data
    tag_seen_cb = cb
    tag_seen_cb_user_data = user_data

def show_tag(test, connector_id: int, tag_id: str):
    global thread_id
    if threading.get_ident() != thread_id:
        work_queue.put((show_tag, [test, connector_id, tag_id]))
        return

    global tag_seen_cb
    global tag_seen_cb_user_data
    test.assertIsNotNone(tag_seen_cb)
    tag_seen_cb(connector_id, tag_id.encode('utf-8'), tag_seen_cb_user_data)

def tick():
    try:
        fn, args = work_queue.get_nowait()
        fn(*args)
    except queue.Empty:
        pass

@ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_uint8)
def platform_tag_rejected(tag, trt):
    pass#print("Tag {} rejected: {}".format(tag.decode('utf-8'), trt))

@ctypes.CFUNCTYPE(None, ctypes.c_int32)
def platform_tag_timed_out(connectorId):
    pass#print("Tag timed out for connector", connectorId)

@ctypes.CFUNCTYPE(None, ctypes.c_int32)
def platform_cable_timed_out(connectorId):
    pass#print("Cable timed out for connector", connectorId)

connector_locked = {}

@ctypes.CFUNCTYPE(None, ctypes.c_int32)
def platform_lock_cable(connectorId):
    connector_locked[connectorId] = True
    #print("Cable locked for connector", connectorId)

@ctypes.CFUNCTYPE(None, ctypes.c_int32)
def platform_unlock_cable(connectorId):
    connector_locked[connectorId] = False
    #print("Cable unlocked for connector", connectorId)

charging_current = {}

@ctypes.CFUNCTYPE(None, ctypes.c_int32, ctypes.c_uint32)
def platform_set_charging_current(connectorId, milliAmps):
    #print("Set charge current to {} for connector {}".format(milliAmps, connectorId))
    charging_current[connectorId] = milliAmps

CONNECTOR_STATE_NOT_CONNECTED = 0
CONNECTOR_STATE_PLUG_DETECTED = 1
CONNECTOR_STATE_CONNECTED = 2
CONNECTOR_STATE_READY_TO_CHARGE = 3
CONNECTOR_STATE_CHARGING = 4
CONNECTOR_STATE_FAULTED = 5

connector_state = {}

@ctypes.CFUNCTYPE(ctypes.c_uint8, ctypes.c_int32)
def platform_get_evse_state(connectorId):
    return connector_state.get(connectorId, 0)


@ctypes.CFUNCTYPE(ctypes.c_char_p, ctypes.c_int32, ctypes.c_uint8)
def platform_get_meter_value(connectorId, measurant):
    return "123.45"

connector_energy = {}

def set_connector_energy(connectorId, milliAmps):
    global thread_id
    if threading.get_ident() != thread_id:
        work_queue.put((set_connector_energy, [connectorId, milliAmps]))
        return

    connector_energy[connectorId] = milliAmps

@ctypes.CFUNCTYPE(ctypes.c_int32, ctypes.c_int32)
def platform_get_energy(connectorId):
    return connector_energy.get(connectorId, 0)

stop_cb = None
stop_cb_user_data = None

@ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.CFUNCTYPE(None, ctypes.c_int32, ctypes.c_int32, ctypes.c_void_p), ctypes.c_void_p)
def platform_register_stop_callback(ctx, cb, user_data):
    global stop_cb
    global stop_cb_user_data
    stop_cb = cb
    stop_cb_user_data = user_data

STOP_REASON_EMERGENCY_STOP = 0 # if EVSE emergency stop button is pressed
STOP_REASON_LOCAL = 1 # "normal" EVSE stop button
STOP_REASON_OTHER = 2
STOP_REASON_POWERLOSS = 3 # "Complete loss of power." maybe use this if contactor check fails (before contactor)
STOP_REASON_REBOOT = 4 # "A locally initiated reset/reboot occurred. (for instance watchdog kicked in)"
STOP_REASON_REMOTE = 5 # "Stopped remotely on request of the user." maybe use this when stopping over the web interface?
def trigger_stop(test, connector_id, reason):
    global thread_id
    if threading.get_ident() != thread_id:
        work_queue.put((trigger_stop, [test, connector_id, reason]))
        return

    global stop_cb
    global stop_cb_user_data
    test.assertIsNotNone(stop_cb)
    stop_cb(connector_id, reason, stop_cb_user_data)

@ctypes.CFUNCTYPE(None)
def platform_reset():
    return print("platform_reset")

files = {}

@ctypes.CFUNCTYPE(ctypes.c_size_t, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char), ctypes.c_size_t)
def platform_read_file(name, buf, len_):
    b = files.get(name, bytearray([0]))
    for i in range(len(b)):
        buf[i] = b[i]
    return len(b)

@ctypes.CFUNCTYPE(ctypes.c_bool, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char), ctypes.c_size_t)
def platform_write_file(name, buf, len_):
    b = (ctypes.c_char * len_).from_address(ctypes.addressof(buf))
    files[name] = list(b)
    return True

def register_default_functions(libocpp):
    libocpp.set_platform_now_ms_cb(platform_now_ms)
    libocpp.set_platform_set_system_time_cb(platform_set_system_time)
    libocpp.set_platform_get_system_time_cb(platform_get_system_time)
    libocpp.set_platform_printfln_cb(platform_printfln)
    libocpp.set_platform_register_tag_seen_callback_cb(platform_register_tag_seen_callback)
    libocpp.set_platform_register_stop_callback_cb(platform_register_stop_callback)
    libocpp.set_platform_tag_rejected_cb(platform_tag_rejected)
    libocpp.set_platform_get_evse_state_cb(platform_get_evse_state)
    libocpp.set_platform_get_meter_value_cb(platform_get_meter_value)
    libocpp.set_platform_get_energy_cb(platform_get_energy)
    libocpp.set_platform_tag_timed_out_cb(platform_tag_timed_out)
    libocpp.set_platform_cable_timed_out_cb(platform_cable_timed_out)
    libocpp.set_platform_lock_cable_cb(platform_lock_cable)
    libocpp.set_platform_unlock_cable_cb(platform_unlock_cable)
    libocpp.set_platform_set_charging_current_cb(platform_set_charging_current)
    libocpp.set_platform_reset_cb(platform_reset)
    libocpp.set_platform_read_file_cb(platform_read_file)
    libocpp.set_platform_write_file_cb(platform_write_file)

def start_client(port, tscale):
    global thread_id
    global timescale
    global time_start

    thread_id = threading.get_ident()
    timescale = tscale
    time_start = time.time()

    libocpp_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "libocpp.so")
    libocpp = ctypes.cdll.LoadLibrary(libocpp_path)
    register_default_functions(libocpp)

    url = "ws://localhost:{}".format(port).encode("utf-8")
    name = "CP_1".encode("utf-8")

    libocpp.ocpp_start(url, name)
    return libocpp

