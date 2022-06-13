import ctypes
import os
import sys
import time

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

@ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_void_p), ctypes.c_void_p)
def platform_register_tag_seen_callback(ctx, cb, user_data):
    tag_seen_cb = cb
    tag_seen_cb_user_data = user_data


@ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_uint8)
def platform_tag_rejected(tag, trt):
    print("Tag {} rejected: {}".format(tag.decode('utf-8'), trt))


@ctypes.CFUNCTYPE(None)
def platform_select_connector():
    pass

select_connector_cb = None
select_connector_cb_user_data = None

@ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.CFUNCTYPE(None, ctypes.c_int32, ctypes.c_void_p), ctypes.c_void_p)
def platform_register_select_connector_callback(ctx, cb, user_data):
    select_connector_cb = cb
    select_connector_cb_user_data = user_data

@ctypes.CFUNCTYPE(ctypes.c_uint8, ctypes.c_int32)
def platform_get_connector_state(connectorId):
    return 0


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
    libocpp.set_platform_select_connector_cb(platform_select_connector)
    libocpp.set_platform_register_select_connector_callback_cb(platform_register_select_connector_callback)
    libocpp.set_platform_get_connector_state_cb(platform_get_connector_state)
    libocpp.set_platform_get_meter_value_cb(platform_get_meter_value)
    libocpp.set_platform_get_energy_cb(platform_get_energy)


def start_client(tscale):
    global timescale
    timescale = tscale
    global time_start
    time_start = time.time()
    libocpp_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "libocpp.so")
    libocpp = ctypes.cdll.LoadLibrary(libocpp_path)
    register_default_functions(libocpp)

    url = "ws://localhost:9000".encode("utf-8")
    name = "CP_1".encode("utf-8")

    libocpp.ocpp_start(url, name)
    return libocpp

