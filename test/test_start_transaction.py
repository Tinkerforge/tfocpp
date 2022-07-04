from datetime import datetime
import unittest
import asyncio
import time
import queue

from ocpp.routing import on, after
from ocpp.v16.enums import Action, RegistrationStatus
from ocpp.v16 import call_result, call
import ocpp.exceptions

import default_platform
import default_central

from test_runner import run_test

class TestStartTransaction(unittest.TestCase):
    """
    The Charge Point SHALL send a StartTransaction.req PDU to the Central System to inform about a transaction
    that has been started.
    """
    def test_start_transaction_sent(test):
        # We don't test for local auth list and auth cache behaviour, only the core profile is supported for now.
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED

            @after(Action.StartTransaction)
            def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    def test_request_params_valid(test):
        class TestCP(default_central.DefaultChargePoint):
            cable_time = None
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_energy[1] = 571339

            @after(Action.Authorize)
            def after_authorize(self, id_tag):
                test.assertEqual(id_tag, "C0:FF:EE")

                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                self.cable_time = self.get_datetime()

            @after(Action.StartTransaction)
            def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                test.assertEqual(connector_id, 1)
                test.assertEqual(id_tag, "C0:FF:EE")
                test.assertEqual(meter_start, 571339)

                # The python datetime parser does not understand Z (or other timezone specifier).
                # We assume for now that the OCPP implementation sends timestamps
                # in the format 2022-07-01T05:33:53Z
                timestamp = timestamp.replace("Z", "+00:00")
                test.assertGreaterEqual(datetime.fromisoformat(timestamp), self.cable_time.replace(microsecond=0))
                test.assertLessEqual(datetime.fromisoformat(timestamp), self.get_datetime())

                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    (Errata 4.0 3.17)
    When the authorization status in the StartTransaction.conf is not Accepted, and the transaction is still ongoing,
    the Charge Point SHOULD
    - when StopTransactionOnInvalidId is set to true: stop the transaction normally as stated in Stop
      Transaction. The Reason field in the Stop Transaction request should be set to DeAuthorized. If the Charge
      Point has the possibility to lock the Charging Cable, it SHOULD keep the Charging Cable locked until the
      owner presents his identifier.
    """
    def test_not_accepted_stop_on_invalid_id(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                await self.call(call.ChangeConfigurationPayload("StopTransactionOnInvalidId", "true"))
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_energy[1] = 571339570

            @on(Action.StartTransaction)
            def on_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                return call_result.StartTransactionPayload(transaction_id=18390123, id_tag_info={"status": "ConcurrentTx"})

            @after(Action.StopTransaction)
            def after_stop_transaction(self, meter_stop: int, timestamp: str, transaction_id: int, **kwargs):
                test.assertEqual(meter_stop, 571339570)
                test.assertEqual(transaction_id, 18390123)
                test.assertEqual(kwargs["reason"], "DeAuthorized")
                test.assertTrue(default_platform.connector_locked[1])
                test.assertEqual(default_platform.charging_current[1], 0)
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    (Errata 4.0 3.5)
    it SHOULD keep the Charging Cable locked until the same identifier (or an identifier with the same ParentIdTag) is used to start the
    transaction (StartTransaction.req) is presented.
    """
    def test_not_accepted_stop_on_invalid_id_dont_unlock_on_other_tag(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                await self.call(call.ChangeConfigurationPayload("StopTransactionOnInvalidId", "true"))
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_energy[1] = 571339570

            @on(Action.StartTransaction)
            def on_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                return call_result.StartTransactionPayload(transaction_id=18390123, id_tag_info={"status": "ConcurrentTx"})

            @after(Action.StopTransaction)
            def after_stop_transaction(self, meter_stop: int, timestamp: str, transaction_id: int, **kwargs):
                test.assertEqual(meter_stop, 571339570)
                test.assertEqual(transaction_id, 18390123)
                test.assertEqual(kwargs["reason"], "DeAuthorized")
                test.assertEqual(default_platform.charging_current[1], 0)
                test.assertTrue(default_platform.connector_locked[1])
                default_platform.show_tag(test, 1, "CA:FE")

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(default_platform.connector_locked[1])
        test.assertEqual(default_platform.charging_current[1], 0)

    def test_not_accepted_stop_on_invalid_id_unlock_on_same_tag(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                await self.call(call.ChangeConfigurationPayload("StopTransactionOnInvalidId", "true"))
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_energy[1] = 571339570

            @on(Action.StartTransaction)
            def on_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                return call_result.StartTransactionPayload(transaction_id=18390123, id_tag_info={"status": "ConcurrentTx"})

            @after(Action.StopTransaction)
            def after_stop_transaction(self, meter_stop: int, timestamp: str, transaction_id: int, **kwargs):
                test.assertEqual(meter_stop, 571339570)
                test.assertEqual(transaction_id, 18390123)
                test.assertEqual(kwargs["reason"], "DeAuthorized")
                test.assertEqual(default_platform.charging_current[1], 0)
                test.assertTrue(default_platform.connector_locked[1])
                default_platform.show_tag(test, 1, "C0:FF:EE")

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertFalse(default_platform.connector_locked[1])
        test.assertEqual(default_platform.charging_current[1], 0)

    """
    - when StopTransactionOnInvalidId is set to false: only stop energy delivery to the vehicle.
        Note: In the case of an invalid identifier, an operator MAY choose to
        charge the EV with a minimum
        amount of energy so the EV is able to drive away. This amount is
        controlled by the optional
        configuration key: MaxEnergyOnInvalidId.
    """
    def test_not_accepted_dont_stop_on_invalid_id(test):
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                await self.call(call.ChangeConfigurationPayload("StopTransactionOnInvalidId", "false"))
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_energy[1] = 571339570

            @on(Action.StartTransaction)
            def on_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                return call_result.StartTransactionPayload(transaction_id=18390123, id_tag_info={"status": "ConcurrentTx"})

            @after(Action.StopTransaction)
            def after_stop_transaction(self, meter_stop: int, timestamp: str, transaction_id: int, **kwargs):
                test.fail("Transaction stopped but StopTransactionOnInvalidId is false")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        # we currently don't implement MaxEnergyOnInvalidId, so just test for 0 here
        test.assertTrue(default_platform.connector_locked[1])
        test.assertEqual(default_platform.charging_current[1], 0)
