import time

import default_central
import default_platform

def run_test(charge_point, sim_len_secs, speedup=1, tick_fn=lambda libocpp, charge_point: True):
    default_central.run_server(charge_point, speedup)

    # Wait until server runs
    while default_central.server is None:
        time.sleep(0.01)

    libocpp = default_platform.start_client(speedup)

    end = time.time() + sim_len_secs / speedup
    while (default_central.active_client is None or not default_central.active_client.done) and time.time() < end: # an ocpp_tick should take ~ 1 ms
        libocpp.ocpp_tick()
        tick_fn(libocpp, charge_point)

    timeout_reached = time.time() < end

    libocpp.ocpp_stop()
    libocpp.ocpp_tick()
    libocpp.ocpp_destroy()

    default_central.server.close()
    while not default_central.done and time.time() < end + 2:
        time.sleep(0.1)

    return timeout_reached, default_central.active_client
