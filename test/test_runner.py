import time
import random

import default_central
import default_platform

def run_test(charge_point_class, sim_len_secs, speedup=1, tick_fn=lambda libocpp, active_client: True):
    port = random.randint(9000, 10000)

    default_central.run_server(port, charge_point_class, speedup)

    # Wait until server runs
    while default_central.server is None:
        time.sleep(0.01)

    libocpp = default_platform.start_client(port, speedup)

    end = time.time() + (sim_len_secs) / speedup
    while (default_central.active_client is None or not default_central.active_client.done) and time.time() < end: # an ocpp_tick should take ~ 1 ms
        if default_central.active_client is not None:
            default_central.active_client.libocpp = libocpp
        libocpp.ocpp_tick()
        default_platform.tick()
        tick_fn(libocpp, default_central.active_client)

    timeout_reached = time.time() >= end

    libocpp.ocpp_stop()
    libocpp.ocpp_tick()
    libocpp.ocpp_destroy()

    default_central.server.close()
    while not default_central.done and time.time() < end + 2:
        time.sleep(0.1)

    return timeout_reached, default_central.active_client
