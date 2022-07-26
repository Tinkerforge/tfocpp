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

class TestChangeAvailability(unittest.TestCase):
    def test_set_unavailable_accepted(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                result = await self.call(call.ChangeAvailabilityPayload(1, "Inoperative"))
                test.assertTrue(result.status == "Accepted")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    A Charge Point is considered unavailable when it does not
    allow any charging.
    """
    def test_no_charging_while_unavailable(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                result = await self.call(call.ChangeAvailabilityPayload(1, "Inoperative"))
                test.assertTrue(result.status == "Accepted")
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED

            @after(Action.StartTransaction)
            def after_start_transaction(self, *args, **kwargs):
                test.fail("Starting a transaction not allowed while unavailable!")

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertFalse(default_platform.connector_locked[1])
        test.assertEqual(default_platform.charging_current[1], 0)

    """
    A Charge Point is considered available
    (“operative”) when it is charging or ready for charging.
    """
    def test_charging_works_again_after_unavailable(test):
        class TestCP(default_central.DefaultChargePoint):
            unavail_done = False
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                result = await self.call(call.ChangeAvailabilityPayload(1, "Inoperative"))
                test.assertTrue(result.status == "Accepted")
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED

            @after(Action.StatusNotification)
            async def after_status_notification(self, connector_id, error_code, status, **kwargs):
                print(connector_id, error_code, status)
                if connector_id == 1 and status == "Unavailable":
                    result = await self.call(call.ChangeAvailabilityPayload(1, "Operative"))
                    test.assertTrue(result.status == "Accepted")
                    self.unavail_done = True
                if self.unavail_done:
                    default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.StartTransaction)
            def after_start_transaction(self, *args, **kwargs):
                self.done = True


        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    In the event that Central System requests Charge Point to change to a status it is already in, Charge Point SHALL
    respond with availability status ‘Accepted’.
    """
    def test_accepted_if_already_in_state(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                result = await self.call(call.ChangeAvailabilityPayload(1, "Operative"))
                test.assertTrue(result.status == "Accepted")

                result = await self.call(call.ChangeAvailabilityPayload(1, "Inoperative"))
                test.assertTrue(result.status == "Accepted")

                result = await self.call(call.ChangeAvailabilityPayload(1, "Inoperative"))
                test.assertTrue(result.status == "Accepted")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    When a transaction is in progress Charge Point SHALL respond with availability status 'Scheduled' to
    indicate that it is scheduled to occur after the transaction has finished.

    When an availability change requested with a ChangeAvailability.req PDU has happened, the Charge Point SHALL
    inform Central System of its new availability status with a StatusNotification.req as described there.
    """
    def test_scheduled_if_txn_is_running(test):
        class TestCP(default_central.DefaultChargePoint):
            txn_stopped = False
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED

            @after(Action.StartTransaction)
            async def after_start_transaction(self, *args, **kwargs):
                result = await self.call(call.ChangeAvailabilityPayload(1, "Inoperative"))
                test.assertTrue(result.status == "Scheduled")
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_PLUG_DETECTED

            @after(Action.StopTransaction)
            def after_stop_transaction(self, *args, **kwargs):
                self.txn_stopped = True

            @after(Action.StatusNotification)
            def after_status_notification(self, connector_id, error_code, status, **kwargs):
                if connector_id == 1 and status == "Unavailable":
                    test.assertTrue(self.txn_stopped)
                    self.done = True

        _, c = run_test(TestCP, sim_len_secs=3, speedup=100)
        test.assertTrue(c.done)

    """
    Tn the case the ChangeAvailability.req contains ConnectorId = 0, the status change applies to
    the Charge Point and all Connectors.
    """
    def test_connector_id_zero(test):
        class TestCP(default_central.DefaultChargePoint):
            seen = 0
            num_connectors = 0
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                self.num_connectors = int((await self.call(call.GetConfigurationPayload(["NumberOfConnectors"]))).configuration_key[0]["value"])

                result = await self.call(call.ChangeAvailabilityPayload(0, "Inoperative"))
                test.assertTrue(result.status == "Accepted")

            @after(Action.StatusNotification)
            def after_status_notification(self, connector_id, error_code, status, **kwargs):
                if status == "Unavailable":
                    self.seen += 1 << connector_id
                    print(self.seen)
                if self.seen == (1 << (self.num_connectors + 1)) - 1:
                    self.done = True

        _, c = run_test(TestCP, sim_len_secs=10, speedup=100)
        test.assertTrue(c.done)
