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

class TestRemoteStopTransaction(unittest.TestCase):
    """
    Central System can request a Charge Point to stop a transaction by sending a RemoteStopTransaction.req to
    Charge Point with the identifier of the transaction. Charge Point SHALL reply with RemoteStopTransaction.conf
    and a status indicating whether it has accepted the request and a transaction with the given transactionId is
    ongoing and will be stopped.

    This remote request to stop a transaction is equal to a local action to stop a transaction. Therefore, the
    transaction SHALL be stopped, The Charge Point SHALL send a StopTransaction.req and, if applicable, unlock the
    connector.
    """
    def test_remote_stop_works(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @on(Action.StartTransaction)
            def on_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                return call_result.StartTransactionPayload(transaction_id=1234, id_tag_info={"status": "Accepted"})

            @after(Action.StartTransaction)
            async def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                resp = await self.call(call.RemoteStopTransactionPayload(1234))
                test.assertEqual(resp.status, "Accepted")

            @after(Action.StopTransaction)
            def after_stop_transaction(self, meter_stop: int, timestamp: str, transaction_id: int, **kwargs):
                test.assertEqual(transaction_id, 1234)
                self.done = True


        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertFalse(default_platform.connector_locked[1])

    def test_remote_stop_rejected_on_invalid_id(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @on(Action.StartTransaction)
            def on_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                return call_result.StartTransactionPayload(transaction_id=1234, id_tag_info={"status": "Accepted"})

            @after(Action.StartTransaction)
            async def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                resp = await self.call(call.RemoteStopTransactionPayload(5678))
                test.assertEqual(resp.status, "Rejected")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertTrue(default_platform.connector_locked[1])

    def test_remote_stop_rejected_if_no_txn_running(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                resp = await self.call(call.RemoteStopTransactionPayload(1234))
                test.assertEqual(resp.status, "Rejected")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

