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

class TestStopTransaction(unittest.TestCase):
    """
    When a transaction is stopped, the Charge Point SHALL send a StopTransaction.req PDU, notifying to the Central
    System that the transaction has stopped.
    """
    def test_stop_transaction_sent(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED

            @after(Action.StartTransaction)
            def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.StopTransaction)
            def after_stop_transaction(self, meter_stop: int, timestamp: str, transaction_id: int, **kwargs):

                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    def test_request_params_valid(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            start_time = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.set_connector_energy(1, 123)
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED

            @after(Action.StartTransaction)
            def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                self.start_time = self.get_datetime().replace(microsecond=0)
                default_platform.set_connector_energy(1, 456)
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.StopTransaction)
            def after_stop_transaction(self, meter_stop: int, timestamp: str, transaction_id: int, **kwargs):
                test.assertEqual(meter_stop, 456)

                timestamp = timestamp.replace("Z", "+00:00")
                test.assertGreaterEqual(datetime.fromisoformat(timestamp), self.start_time)
                test.assertLessEqual(datetime.fromisoformat(timestamp), self.get_datetime())

                test.assertEqual(transaction_id, 1234) # DefaultChargePoint returns this on starttransaction

                self.done = True


        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    As part of the normal transaction termination, the
    Charge Point SHALL unlock the cable (if not permanently attached).
    """
    def test_cable_unlock(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED

            @after(Action.StartTransaction)
            def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.StopTransaction)
            def after_stop_transaction(self, meter_stop: int, timestamp: str, transaction_id: int, **kwargs):
                test.assertFalse(default_platform.connector_locked[1])
                test.assertEqual(default_platform.charging_current[1], 0)
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    If a transaction is ended in a normal way (e.g. EV-driver presented his identification to stop the transaction), the
    Reason element MAY be omitted and the Reason SHOULD be assumed 'Local'.
    """
    def test_stop_transaction_optional_reason_on_local_stop(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED

            @after(Action.StartTransaction)
            def after_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.StopTransaction)
            def after_stop_transaction(self, meter_stop: int, timestamp: str, transaction_id: int, **kwargs):
                test.assertEqual(kwargs.get("reason", "Local"), "Local")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    If the transaction is not ended
    normally, the Reason SHOULD be set to a correct value.
    """
    #todo test all reasons. Add functions to platform interface to trigger those reasons. Maybe "platform_set_stop_callback" and then cb(StopReason.EmergencyStop) etc.
    @unittest.skip("Not implemented yet")
    def test_all_reasons(test):
        pass


    """
    The Charge Point MAY unlock the cable (if not permanently attached) when the cable is disconnected at the EV. If
    supported, this functionality is reported and controlled by the configuration key
    UnlockConnectorOnEVSideDisconnect.
    """
    @unittest.skip("Not implemented yet")
    def test_unlock_on_ev_disconnect(test):
        pass


    """
    By setting StopTransactionOnEVSideDisconnect to true, the transaction SHALL be stopped when the cable
    is disconnected from the EV. If the EV is reconnected, energy transfer is not allowed until the transaction is
    stopped and a new transaction is started
    """
    @unittest.skip("Not implemented yet")
    def test_stop_on_ev_disconnect(test):
        pass
    # false is not supported, see errata 4.0. implement this key (it is optional but we don't want to trip centrals that expect it to be required) as readonly, always true
