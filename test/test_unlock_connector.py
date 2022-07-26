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

class TestUnlockConnector(unittest.TestCase):
    """
    Upon receipt of an UnlockConnector.req PDU, the Charge Point SHALL respond with a UnlockConnector.conf
    PDU. The response PDU SHALL indicate whether the Charge Point was able to unlock its connector.
    """
    def test_unlock_works(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.StartTransaction)
            def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                test.assertTrue(default_platform.connector_locked[1])
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_PLUG_DETECTED

            @after (Action.StopTransaction)
            async def after_stop_transaction(self, *args, **kwargs):
                test.assertTrue(default_platform.connector_locked[1])

                resp = await self.call(call.UnlockConnectorPayload(1))
                test.assertEqual(resp.status, "Unlocked")

                test.assertFalse(default_platform.connector_locked[1])
                self.done = True


        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertFalse(default_platform.connector_locked[1])

    """
    If there was a transaction in progress on the specific connector, then Charge Point SHALL finish the transaction
    first as described in Stop Transaction.
    """
    def test_unlock_sends_stop_txn(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.StartTransaction)
            async def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                test.assertTrue(default_platform.connector_locked[1])

                resp = await self.call(call.UnlockConnectorPayload(1))
                test.assertEqual(resp.status, "Unlocked")

                test.assertFalse(default_platform.connector_locked[1])

            @after (Action.StopTransaction)
            def after_stop_transaction(self, *args, **kwargs):
                self.done = True


        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertFalse(default_platform.connector_locked[1])


    """
    NotSupported: Charge Point has no connector lock, or ConnectorId is unknown.
    """
    def test_unlock_not_supported_on_invalid_connector_id(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.StartTransaction)
            async def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                test.assertTrue(default_platform.connector_locked[1])

                resp = await self.call(call.UnlockConnectorPayload(0))
                test.assertEqual(resp.status, "NotSupported")

                resp = await self.call(call.UnlockConnectorPayload(-1))
                test.assertEqual(resp.status, "NotSupported")

                resp = await self.call(call.UnlockConnectorPayload(2))
                test.assertEqual(resp.status, "NotSupported")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertTrue(default_platform.connector_locked[1])
