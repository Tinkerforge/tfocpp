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
    def test_get_all_values_empty_list(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload([])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        test.assertTrue(c.task.done())
        test.assertTrue(c.task.result().unknown_key is None or len(c.task.result().unknown_key) == 0)

        result = c.task.result()
        # At least all core profile config keys must be returned.
        # Further tests (i.e. about writeability, correctness and completeness of keys etc.)
        # is done in the profile specific tests
        test.assertGreaterEqual(len(result.configuration_key), 21) # 21 keys of the core profile are required

    def test_get_all_values_omitted_list(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload()))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        test.assertTrue(c.task.done())
        test.assertTrue(c.task.result().unknown_key is None or len(c.task.result().unknown_key) == 0)

        result = c.task.result()
        # At least all core profile config keys must be returned.
        # Further tests (i.e. about writeability, correctness and completeness of keys etc.)
        # is done in the profile specific tests
        test.assertGreaterEqual(len(result.configuration_key), 21) # 21 keys of the core profile are required

    """
    Otherwise Charge Point SHALL return a list of recognized keys
    and their corresponding values and read-only state.
    """
    def test_get_single_value(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload(["HeartbeatInterval"])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        test.assertTrue(c.task.done())
        test.assertTrue(c.task.result().unknown_key is None or len(c.task.result().unknown_key) == 0)

        result = c.task.result()
        test.assertEqual(int(result.configuration_key[0]["value"]), 10)

    def test_get_multiple_values(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload(["HeartbeatInterval", "LocalAuthorizeOffline", "ConnectorPhaseRotation"])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)


        test.assertTrue(c.task.done())
        test.assertTrue(c.task.result().unknown_key is None or len(c.task.result().unknown_key) == 0)

        ck =  c.task.result().configuration_key
        test.assertGreaterEqual(len(ck), 3, "three known keys requested")

        # OCPP does not specify the result array order, so testing this is a bit more complicated

        test.assertEqual(sorted([ck[x]["key"] for x in range(len(ck))]), sorted(["HeartbeatInterval", "LocalAuthorizeOffline", "ConnectorPhaseRotation"]))

        test.assertEqual(ck[0]["readonly"], False)
        test.assertEqual(ck[1]["readonly"], False)
        test.assertEqual(ck[2]["readonly"], False)

        test.assertIsInstance(ck[0]["value"], str, "All values must be passed as string")
        test.assertIsInstance(ck[1]["value"], str, "All values must be passed as string")
        test.assertIsInstance(ck[2]["value"], str, "All values must be passed as string")

    def test_single_unknown_value(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload(["Unknown1"])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        test.assertTrue(c.task.done())
        test.assertTrue(c.task.result().configuration_key is None or len(c.task.result().configuration_key) == 0)

        result = c.task.result()
        test.assertEqual(len(result.unknown_key), 1)
        test.assertEqual(result.unknown_key[0], "Unknown1")

    def test_multiple_unknown_values(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload(["Unknown1", "Unknown2", "Unknown3", "Unknown4"])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        test.assertTrue(c.task.done())
        test.assertTrue(c.task.result().configuration_key is None or len(c.task.result().configuration_key) == 0)

        result = c.task.result()
        test.assertEqual(len(result.unknown_key), 4)
        test.assertEqual(result.unknown_key[0], "Unknown1")
        test.assertEqual(result.unknown_key[1], "Unknown2")
        test.assertEqual(result.unknown_key[2], "Unknown3")
        test.assertEqual(result.unknown_key[3], "Unknown4")

    def test_mixed(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload(["HeartbeatInterval", "Unknown1", "Unknown2", "LocalAuthorizeOffline", "ConnectorPhaseRotation", "Unknown3", "Unknown4"])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        test.assertTrue(c.task.done())

        result = c.task.result()
        test.assertEqual(len(result.unknown_key), 4)
        test.assertEqual(result.unknown_key[0], "Unknown1")
        test.assertEqual(result.unknown_key[1], "Unknown2")
        test.assertEqual(result.unknown_key[2], "Unknown3")
        test.assertEqual(result.unknown_key[3], "Unknown4")

        ck =  c.task.result().configuration_key
        test.assertGreaterEqual(len(ck), 3, "three known keys requested")

        # OCPP does not specify the result array order, so testing this is a bit more complicated

        test.assertEqual(sorted([ck[x]["key"] for x in range(len(ck))]), sorted(["HeartbeatInterval", "LocalAuthorizeOffline", "ConnectorPhaseRotation"]))

        test.assertEqual(ck[0]["readonly"], False)
        test.assertEqual(ck[1]["readonly"], False)
        test.assertEqual(ck[2]["readonly"], False)

        test.assertIsInstance(ck[0]["value"], str, "All values must be passed as string")
        test.assertIsInstance(ck[1]["value"], str, "All values must be passed as string")
        test.assertIsInstance(ck[2]["value"], str, "All values must be passed as string")

    """
    The number of configuration keys requested in a single PDU MAY be limited by the Charge Point. This maximum
    can be retrieved by reading the configuration key GetConfigurationMaxKeys.
    """
    def test_max_keys(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            all_keys = None
            max_keys = None

            @after(Action.BootNotification)
            async def after_boot_notification(inner_self, *args, **kwargs):
                conf = (await inner_self.call(call.GetConfigurationPayload(["GetConfigurationMaxKeys"]))).configuration_key[0]
                test.assertEqual(conf["key"], "GetConfigurationMaxKeys")
                inner_self.max_keys = int(conf["value"])
                inner_self.all_keys = await inner_self.call(call.GetConfigurationPayload([]))
                inner_self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        test.assertLessEqual(len(c.all_keys.configuration_key), c.max_keys)
