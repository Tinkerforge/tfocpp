from datetime import datetime
import unittest
import asyncio
import time

from ocpp.routing import on, after
from ocpp.v16.enums import Action, RegistrationStatus
from ocpp.v16 import call_result, call
import ocpp.exceptions

import default_platform
import default_central

from test_runner import run_test

class TestHeartbeat(unittest.TestCase):
    """
    The Charge Point SHALL send a Heartbeat.req PDU for ensuring that the Central System knows that a Charge
    Point is still alive.
    """
    def test_heartbeat_sent(self):
        class TestCP(default_central.DefaultChargePoint):
            @on(Action.BootNotification)
            def on_boot_notification(self, charge_point_vendor: str, charge_point_model: str, **kwargs):
                return call_result.BootNotificationPayload(
                    current_time=self.get_datetime().isoformat(),
                    interval=1,
                    status=RegistrationStatus.accepted
                )
        _, c = run_test(TestCP, sim_len_secs=10, speedup=100)

        self.assertEqual(c.received_calls[Action.Heartbeat], 9, "Expected exactly nine heartbeats")

    """
    The response
    PDU SHALL contain the current time of the Central System, which is RECOMMENDED to be used by the Charge
    Point to synchronize its internal clock.
    """
    def test_clock_adjusted(self):
        class TestCP(default_central.DefaultChargePoint):
            sent_time = 0

            @on(Action.BootNotification)
            def on_boot_notification(self, charge_point_vendor: str, charge_point_model: str, **kwargs):
                return call_result.BootNotificationPayload(
                    current_time=self.get_datetime().isoformat(),
                    interval=1,
                    status=RegistrationStatus.accepted
                )

            @on(Action.Heartbeat)
            def on_heartbeat(self, *args, **kwargs):
                t = self.get_datetime()
                self.sent_time = int(t.timestamp())

                return call_result.HeartbeatPayload(t.isoformat())

        start = time.time()
        _, c = run_test(TestCP, 10, speedup=100)
        self.assertGreaterEqual(default_platform.last_time_set_at, start)
        self.assertEqual(default_platform.last_time_set, c.sent_time)
