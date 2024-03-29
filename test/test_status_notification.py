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

class TestStatusNotification(unittest.TestCase):
    """
    (Errata 4.0)
    After the Central System accept a Charge Point by sending a BootNotification.conf with a status Accepted, the Charge Point
    SHALL send a StatusNotification.req PDU for connectorId 0 and all connectors with the current status.
    """
    def test_available_on_boot(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload(["NumberOfConnectors"])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.task.done())
        num_connectors = int(c.task.result().configuration_key[0]["value"])

        test.assertEqual(len(c.status), num_connectors + 1)
        test.assertEqual(list(c.status.values()), [["Available"]] * (num_connectors + 1))

    """
    Typically a Connector is in preparing state when a user [...] inserts a cable [...]
    """
    def test_notify_preparing_on_connect(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_PLUG_DETECTED

            @after(Action.StatusNotification)
            def after_status_notification(self, connector_id, error_code, status, **kwargs):
                if connector_id == 1 and status == "Preparing":
                    self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertEqual(c.status[1], ["Available", "Preparing"])

    """
    Typically a Connector is in preparing state when a user presents a tag [...]
    """
    def test_notify_preparing_on_tag(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.StatusNotification)
            def after_status_notification(self, connector_id, error_code, status, **kwargs):
                if connector_id != 1:
                    return

                if status == "Preparing":
                    self.done = True
                else:
                    default_platform.show_tag(test, 1, "C0:FF:EE")

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertEqual(c.status[1], ["Available", "Preparing"])


    """
    When the EV is connected to the EVSE but the EVSE is not offering energy to the EV, e.g. due to a smart charging restriction,
    local supply power constraints, or as the result of StartTransaction.conf indicating that charging is not allowed etc.
    """

    def test_notify_suspended_evse_on_slot_blocked(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED


            @after(Action.StatusNotification)
            def after_status_notification(self, connector_id, error_code, status, **kwargs):
                if connector_id == 1 and status == "SuspendedEVSE":
                    self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertEqual(c.status[1], ["Available", "Preparing", "SuspendedEVSE"])


    """
    When the EV is connected to the EVSE but the EVSE is not offering energy to the EV, e.g. due to a smart charging restriction,
    local supply power constraints, or as the result of StartTransaction.conf indicating that charging is not allowed etc.
    """
    def test_notify_suspended_evse_on_start_transaction_blocked(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @on(Action.StartTransaction)
            def on_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                return call_result.StartTransactionPayload(transaction_id=1234, id_tag_info={"status": "ConcurrentTx"})

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED


            @after(Action.StatusNotification)
            def after_status_notification(self, connector_id, error_code, status, **kwargs):
                if connector_id != 1:
                    return

                if status == "Preparing":
                    default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CHARGING
                elif status == "SuspendedEVSE":
                    self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertEqual(c.status[1], ["Available", "Preparing", "Charging", "SuspendedEVSE"])

    """
    When the EV is connected to the EVSE and the EVSE is offering energy but the EV is not taking any energy.
    """
    def test_notify_suspended_ev(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED


            @after(Action.StatusNotification)
            def after_status_notification(self, connector_id, error_code, status, **kwargs):
                if connector_id != 1:
                    return

                if status == "Preparing":
                    default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_READY_TO_CHARGE
                elif status == "SuspendedEV":
                    self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertEqual(c.status[1], ["Available", "Preparing", "SuspendedEV"])

    """
    When a Transaction has stopped at a Connector, but the Connector is not yet available for a new user, e.g. the cable has not been removed [...]
    """
    def test_notify_finishing(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED


            @after(Action.StatusNotification)
            def after_status_notification(self, connector_id, error_code, status, **kwargs):
                if connector_id != 1:
                    return

                if status == "Preparing":
                    default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CHARGING
                elif status == "Charging":
                    default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_PLUG_DETECTED
                elif status == "Finishing":
                    self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertEqual(c.status[1], ["Available", "Preparing", "Charging", "Finishing"])


    """
    When a Connector becomes reserved as a result of a Reserve Now command
    """
    @unittest.skip("Not implemented yet. Not core profile")
    def test_notify_reserved(test):
        pass

    """
    When a Connector becomes unavailable as the result of a Change Availability command or an event upon which the Charge
    Point transitions to unavailable at its discretion. Upon receipt of a Change Availability command, the status MAY change
    immediately or the change MAY be scheduled. When scheduled, the Status Notification shall be send when the availability
    change becomes effective
    """
    @unittest.skip("Not implemented yet")
    def test_notify_unavailable(test):
        # todo: "an event upon which the Charge Point transitions to unavailable at its discretion" does this ever happen?
        pass


    """
    If charging is suspended both by the EV and the EVSE, status SuspendedEVSE SHALL have
    precedence over status SuspendedEV.
    """
    def test_suspended_precedence(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @on(Action.StartTransaction)
            def on_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
                return call_result.StartTransactionPayload(transaction_id=1234, id_tag_info={"status": "ConcurrentTx"})

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED


            @after(Action.StatusNotification)
            def after_status_notification(self, connector_id, error_code, status, **kwargs):
                if connector_id != 1:
                    return

                if status == "Preparing":
                    default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_READY_TO_CHARGE
                elif status == "SuspendedEVSE":
                    self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        #test.assertTrue(c.done)
        test.assertEqual(c.status[1], ["Available", "Preparing", "SuspendedEV", "SuspendedEVSE"])

    """
    When a Charge Point or a Connector is set to status Unavailable by a Change Availability
    command, the 'Unavailable' status MUST be persistent across reboots.
    """
    @unittest.skip("Not implemented yet")
    def test_unavailable_persistence(test):
        pass

    """
    the Charge Point MAY omit sending a StatusNotification.req if it was active for
    less time than defined in the optional configuration key MinimumStatusDuration. This way, a Charge Point
    MAY choose not to send certain StatusNotification.req PDUs.
    """
    @unittest.skip("Not implemented yet")
    def test_minimum_status_duration(test):
        pass

    """
    ChargePointErrorCode EVCommunicationError SHALL only be used with status
    SuspendedEV, SuspendedEVSE and Finishing and be treated as warning.
    """
    @unittest.skip("Not implemented yet")
    def test_ev_communication_error(test):
        pass

    """
    When a Charge Point connects to a Central System after having been offline, it updates the Central System about
    its status according to the following rules:

    1. The Charge Point SHOULD send a StatusNotification.req PDU with its current status if the status changed
       while the Charge Point was offline.
    2. The Charge Point MAY send a StatusNotification.req PDU to report an error that occurred while the
       Charge Point was offline.
    3. The Charge Point SHOULD NOT send StatusNotification.req PDUs for historical status change events that
       happened while the Charge Point was offline and that do not inform the Central System of Charge Point
       errors or the Charge Point’s current status.
    4. The StatusNotification.req messages MUST be sent in the order in which the events that they describe
       occurred.
    """
    @unittest.skip("Not implemented yet")
    def test_offline(test):
        pass

    """
    PowerSwitchFailure: Failure to control power switch.
    """
    @unittest.skip("Not implemented yet")
    def test_notify_on_contactor_error(test):
        pass

    """
    GroundFailure Ground fault circuit interrupter has been activated.
    """
    @unittest.skip("Not implemented yet")
    def test_notify_on_dc_error(test):
        # write dc error state into info
        pass

    """
    EVCommunicationError
    Communication failure with the vehicle, might be Mode 3 or other communication protocol problem. This is
    not a real error in the sense that the Charge Point doesn’t need to go to the faulted state. Instead, it should go
    to the SuspendedEVSE state.
    """
    @unittest.skip("Not implemented yet")
    def test_notify_on_communication_error(test):
        pass

    """
    InternalError
    Error in internal hard- or software component.
    """
    @unittest.skip("Not implemented yet")
    def test_notify_on_switch_error(test):
        # Use this for switch error state reported by EVSE
        pass

    """
    PowerMeterFailure Failure to read electrical/energy/power meter.
    """
    @unittest.skip("Not implemented yet")
    def test_notify_on_power_meter_failure(test):
        pass

    """
    ReaderFailure Failure with idTag reader.
    """
    @unittest.skip("Not implemented yet")
    def test_notify_on_idtag_reader_failure(test):
        pass

    """
    WeakSignal  Wireless communication device reports a weak signal.
    """
    @unittest.skip("Not implemented yet")
    def test_notify_on_weak_signal(test):
        pass

