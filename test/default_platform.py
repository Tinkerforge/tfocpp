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
    print("Tag {} rejected: {}".format(tag.decode('utf-8'), trt))

@ctypes.CFUNCTYPE(None, ctypes.c_int32)
def platform_tag_timed_out(connectorId):
    print("Tag timed out for connector", connectorId)

@ctypes.CFUNCTYPE(None, ctypes.c_int32)
def platform_cable_timed_out(connectorId):
    print("Cable timed out for connector", connectorId)

@ctypes.CFUNCTYPE(None, ctypes.c_int32)
def platform_lock_cable(connectorId):
    print("Cable locked for connector", connectorId)

@ctypes.CFUNCTYPE(None, ctypes.c_int32)
def platform_unlock_cable(connectorId):
    print("Cable unlocked for connector", connectorId)

charging_current = {}

@ctypes.CFUNCTYPE(None, ctypes.c_int32, ctypes.c_uint32)
def platform_set_charging_current(connectorId, milliAmps):
    print("Set charge current to {} for connector {}".format(milliAmps, connectorId))
    charging_current[connectorId] = milliAmps

CONNECTOR_STATE_NOT_CONNECTED = 0
CONNECTOR_STATE_CONNECTED = 1
CONNECTOR_STATE_READY_TO_CHARGE = 2
CONNECTOR_STATE_CHARGING = 3
CONNECTOR_STATE_FAULTED = 4

connector_state = {}

@ctypes.CFUNCTYPE(ctypes.c_uint8, ctypes.c_int32)
def platform_get_evse_state(connectorId):
    return connector_state.get(connectorId, 0)


@ctypes.CFUNCTYPE(ctypes.c_char_p, ctypes.c_int32, ctypes.c_uint8)
def platform_get_meter_value(connectorId, measurant):
    return "123.45"


@ctypes.CFUNCTYPE(ctypes.c_int32, ctypes.c_int32)
def platform_get_energy(connectorId):
    return 1234


def register_default_functions(libocpp):
    libocpp.set_platform_now_ms_cb(platform_now_ms)
    libocpp.set_platform_set_system_time_cb(platform_set_system_time)
    libocpp.set_platform_get_system_time_cb(platform_get_system_time)
    libocpp.set_platform_printfln_cb(platform_printfln)
    libocpp.set_platform_register_tag_seen_callback_cb(platform_register_tag_seen_callback)
    libocpp.set_platform_tag_rejected_cb(platform_tag_rejected)
    libocpp.set_platform_get_evse_state_cb(platform_get_evse_state)
    libocpp.set_platform_get_meter_value_cb(platform_get_meter_value)
    libocpp.set_platform_get_energy_cb(platform_get_energy)
    libocpp.set_platform_tag_timed_out_cb(platform_tag_timed_out)
    libocpp.set_platform_cable_timed_out_cb(platform_cable_timed_out)
    libocpp.set_platform_lock_cable_cb(platform_lock_cable)
    libocpp.set_platform_unlock_cable_cb(platform_unlock_cable)
    libocpp.set_platform_set_charging_current_cb(platform_set_charging_current)

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

