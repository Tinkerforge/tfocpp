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

class TestRemoteStartTransaction(unittest.TestCase):
    """
    If the value of AuthorizeRemoteTxRequests is false, the Charge Point SHALL immediately try to start a
    transaction for the idTag given in the RemoteStartTransaction.req message.
    """
    def test_remote_start_works(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                test.assertEqual((await self.call(call.ChangeConfigurationPayload("AuthorizeRemoteTxRequests", "false"))).status, "Accepted")
                result = await self.call(call.RemoteStartTransactionPayload("00:11:22", 1))
                test.assertTrue(result.status == "Accepted")

            @after(Action.StartTransaction)
            def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                test.assertEqual(connector_id, 1)
                test.assertEqual(id_tag, "00:11:22")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    If the value of AuthorizeRemoteTxRequests is true, the Charge Point SHALL behave as if in response to
    a local action at the Charge Point to start a transaction with the idTag given in the
    RemoteStartTransaction.req message.
    """
    def test_authorize_remote_txn_reqs(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                test.assertEqual((await self.call(call.ChangeConfigurationPayload("AuthorizeRemoteTxRequests", "true"))).status, "Accepted")

                result = await self.call(call.RemoteStartTransactionPayload("00:11:22", 1))
                test.assertTrue(result.status == "Accepted")

            @after(Action.Authorize)
            def after_authorize(self, id_tag):
                test.assertEqual(id_tag, "00:11:22")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    A transaction will only be started after authorization was obtained.
    """
    def test_authorize_fail_blocks_txn(test):
        class TestCP(default_central.DefaultChargePoint):
            start_txn_recvd = False
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                test.assertEqual((await self.call(call.ChangeConfigurationPayload("AuthorizeRemoteTxRequests", "true"))).status, "Accepted")

                result = await self.call(call.RemoteStartTransactionPayload("00:11:22", 1))
                test.assertTrue(result.status == "Accepted")

            @on(Action.Authorize)
            def on_authorize(self, id_tag):
                return call_result.AuthorizePayload(id_tag_info={"status": "Invalid"})

            @after(Action.StartTransaction)
            def after_start_transaction(self, *args, **kwargs):
                self.start_txn_recvd = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertFalse(c.start_txn_recvd)

    """
    The RemoteStartTransaction.req SHALL contain an identifier (idTag), which Charge Point SHALL use, if it is able to
    start a transaction, to send a StartTransaction.req to Central System.
    """
    def test_override_tag_id(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                test.assertEqual((await self.call(call.ChangeConfigurationPayload("AuthorizeRemoteTxRequests", "false"))).status, "Accepted")
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.Authorize)
            async def after_authorize(self, id_tag):
                test.assertEqual(id_tag, "C0:FF:EE")
                result = await self.call(call.RemoteStartTransactionPayload("00:11:22", 1))
                test.assertTrue(result.status == "Accepted")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED

            @after(Action.StartTransaction)
            def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                test.assertEqual(id_tag, "00:11:22")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    When no connector id is provided, the Charge Point is in control of the
    connector selection. A Charge Point MAY reject a RemoteStartTransaction.req without a connector id.
    """
    def test_no_connector_id(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                test.assertEqual((await self.call(call.ChangeConfigurationPayload("AuthorizeRemoteTxRequests", "false"))).status, "Accepted")
                result = await self.call(call.RemoteStartTransactionPayload("00:11:22"))
                test.assertTrue(result.status == "Accepted")

            @after(Action.StartTransaction)
            def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                test.assertEqual(connector_id, 1)
                test.assertEqual(id_tag, "00:11:22")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    def test_reject_on_invalid_connector_id(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                test.assertEqual((await self.call(call.ChangeConfigurationPayload("AuthorizeRemoteTxRequests", "false"))).status, "Accepted")

                result = await self.call(call.RemoteStartTransactionPayload("00:11:22", 0))
                test.assertTrue(result.status == "Rejected")

                result = await self.call(call.RemoteStartTransactionPayload("00:11:22", 2))
                test.assertTrue(result.status == "Rejected")

                result = await self.call(call.RemoteStartTransactionPayload("00:11:22", -1))
                test.assertTrue(result.status == "Rejected")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
