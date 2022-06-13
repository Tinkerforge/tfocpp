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

class TestGetConfiguration(unittest.TestCase):
    """
    If the list of keys in the request PDU is empty or missing (it is optional), the Charge Point SHALL return a list of all
    configuration settings
    """
    def test_get_all_values_empty_list(self):
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload([])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        self.assertTrue(c.task.done())
        self.assertTrue(c.task.result().unknown_key is None or len(c.task.result().unknown_key) == 0)

        result = c.task.result()
        # At least all core profile config keys must be returned.
        # Further tests (i.e. about writeability, correctness and completeness of keys etc.)
        # is done in the profile specific tests
        self.assertGreaterEqual(len(result.configuration_key), 21) # 21 keys of the core profile are required

    def test_get_all_values_omitted_list(self):
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload()))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        self.assertTrue(c.task.done())
        self.assertTrue(c.task.result().unknown_key is None or len(c.task.result().unknown_key) == 0)

        result = c.task.result()
        # At least all core profile config keys must be returned.
        # Further tests (i.e. about writeability, correctness and completeness of keys etc.)
        # is done in the profile specific tests
        self.assertGreaterEqual(len(result.configuration_key), 21) # 21 keys of the core profile are required

    """
    Otherwise Charge Point SHALL return a list of recognized keys
    and their corresponding values and read-only state.
    """
    def test_get_single_value(self):
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload(["HeartbeatInterval"])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        self.assertTrue(c.task.done())
        self.assertTrue(c.task.result().unknown_key is None or len(c.task.result().unknown_key) == 0)

        result = c.task.result()
        self.assertEqual(int(result.configuration_key[0]["value"]), 10)

    def test_get_multiple_values(self):
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload(["HeartbeatInterval", "LocalAuthorizeOffline", "ConnectorPhaseRotation"])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)


        self.assertTrue(c.task.done())
        self.assertTrue(c.task.result().unknown_key is None or len(c.task.result().unknown_key) == 0)

        ck =  c.task.result().configuration_key
        self.assertGreaterEqual(len(ck), 3, "three known keys requested")

        # OCPP does not specify the result array order, so testing this is a bit more complicated

        self.assertEqual(sorted([ck[x]["key"] for x in range(len(ck))]), sorted(["HeartbeatInterval", "LocalAuthorizeOffline", "ConnectorPhaseRotation"]))

        self.assertEqual(ck[0]["readonly"], False)
        self.assertEqual(ck[1]["readonly"], False)
        self.assertEqual(ck[2]["readonly"], False)

        self.assertIsInstance(ck[0]["value"], str, "All values must be passed as string")
        self.assertIsInstance(ck[1]["value"], str, "All values must be passed as string")
        self.assertIsInstance(ck[2]["value"], str, "All values must be passed as string")

    def test_single_unknown_value(self):
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload(["Unknown1"])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        self.assertTrue(c.task.done())
        self.assertTrue(c.task.result().configuration_key is None or len(c.task.result().configuration_key) == 0)

        result = c.task.result()
        self.assertEqual(len(result.unknown_key), 1)
        self.assertEqual(result.unknown_key[0], "Unknown1")

    def test_multiple_unknown_values(self):
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload(["Unknown1", "Unknown2", "Unknown3", "Unknown4"])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        self.assertTrue(c.task.done())
        self.assertTrue(c.task.result().configuration_key is None or len(c.task.result().configuration_key) == 0)

        result = c.task.result()
        self.assertEqual(len(result.unknown_key), 4)
        self.assertEqual(result.unknown_key[0], "Unknown1")
        self.assertEqual(result.unknown_key[1], "Unknown2")
        self.assertEqual(result.unknown_key[2], "Unknown3")
        self.assertEqual(result.unknown_key[3], "Unknown4")

    def test_mixed(self):
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload(["HeartbeatInterval", "Unknown1", "Unknown2", "LocalAuthorizeOffline", "ConnectorPhaseRotation", "Unknown3", "Unknown4"])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        self.assertTrue(c.task.done())

        result = c.task.result()
        self.assertEqual(len(result.unknown_key), 4)
        self.assertEqual(result.unknown_key[0], "Unknown1")
        self.assertEqual(result.unknown_key[1], "Unknown2")
        self.assertEqual(result.unknown_key[2], "Unknown3")
        self.assertEqual(result.unknown_key[3], "Unknown4")

        ck =  c.task.result().configuration_key
        self.assertGreaterEqual(len(ck), 3, "three known keys requested")

        # OCPP does not specify the result array order, so testing this is a bit more complicated

        self.assertEqual(sorted([ck[x]["key"] for x in range(len(ck))]), sorted(["HeartbeatInterval", "LocalAuthorizeOffline", "ConnectorPhaseRotation"]))

        self.assertEqual(ck[0]["readonly"], False)
        self.assertEqual(ck[1]["readonly"], False)
        self.assertEqual(ck[2]["readonly"], False)

        self.assertIsInstance(ck[0]["value"], str, "All values must be passed as string")
        self.assertIsInstance(ck[1]["value"], str, "All values must be passed as string")
        self.assertIsInstance(ck[2]["value"], str, "All values must be passed as string")

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
