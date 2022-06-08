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

# Between the physical power-on/reboot and the successful completion of a BootNotification,
# where Central System returns Accepted or Pending,
# the Charge Point SHALL NOT send any other request to the Central System.
def assert_no_other_packets(test_case, c):
    test_case.assertEqual(len(c.received_calls), 1, "Expected no other received calls")
    test_case.assertEqual(len(c.received_results), 0, "Expected no received results")
    test_case.assertEqual(len(c.received_errors), 0, "Expected no received errors")

class TestBootNotification(unittest.TestCase):
    def test_req_sent(self):
        _, c = run_test(default_central.DefaultChargePoint, sim_len_secs=10, speedup=100)

        self.assertEqual(c.received_calls[Action.BootNotification], 1, "Expected exactly one boot notification if it is accepted")
        assert_no_other_packets(self, c)

    """
    If the Central System returns
    something other than Accepted, the value of the interval field indicates the minimum wait time before sending a
    next BootNotification request.
    """
    def test_retry_on_pending(self):
        class TestCP(default_central.DefaultChargePoint):
            @on(Action.BootNotification)
            def on_boot_notification(self, charge_point_vendor: str, charge_point_model: str, **kwargs):
                return call_result.BootNotificationPayload(
                    current_time=self.get_datetime().isoformat(),
                    interval=1,
                    status=RegistrationStatus.pending
                )

        _, c = run_test(TestCP, 10, speedup=100)
        self.assertEqual(c.received_calls[Action.BootNotification], 10, "Expected one boot notification resend per interval")
        assert_no_other_packets(self, c)

    def test_retry_on_rejected(self):
        class TestCP(default_central.DefaultChargePoint):
            @on(Action.BootNotification)
            def on_boot_notification(self, charge_point_vendor: str, charge_point_model: str, **kwargs):
                return call_result.BootNotificationPayload(
                    current_time=self.get_datetime().isoformat(),
                    interval=1,
                    status=RegistrationStatus.rejected
                )

        _, c = run_test(TestCP, 10, speedup=100)
        self.assertEqual(c.received_calls[Action.BootNotification], 10, "Expected one boot notification resend per interval")
        assert_no_other_packets(self, c)

    """
    If that interval value is zero, the Charge Point chooses a waiting interval on its
    own, in a way that avoids flooding the Central System with requests.
    """
    def test_interval_zero(self):
        class TestCP(default_central.DefaultChargePoint):
            @on(Action.BootNotification)
            def on_boot_notification(self, charge_point_vendor: str, charge_point_model: str, **kwargs):
                return call_result.BootNotificationPayload(
                    current_time=self.get_datetime().isoformat(),
                    interval=0,
                    status=RegistrationStatus.pending
                )

        _, c = run_test(TestCP, 100, speedup=100)
        # don't flood := at most one notification every 10 seconds
        self.assertLessEqual(c.received_calls[Action.BootNotification], 10, "Expected one boot notification resend per interval")
        assert_no_other_packets(self, c)

    """
     If the Central System returns something other than Accepted[...] A Charge Point SHOULD NOT send a
    BootNotification.req earlier, unless requested to do so with a TriggerMessage.req.
    """
    @unittest.skip("Not implemented yet")
    def test_respond_to_trigger_message(self):
        # configure interval longer than test time
        # send trigger message after short interval
        # check if two notifications were received (one on boot up, one on trigger)
        pass

    # While Rejected, the Charge Point SHALL NOT respond to any Central System initiated message.
    def test_dont_respond_while_rejected(self):
        class TestCP(default_central.DefaultChargePoint):
            get_conf_task = None

            @on(Action.BootNotification)
            def on_boot_notification(self, charge_point_vendor: str, charge_point_model: str, **kwargs):
                return call_result.BootNotificationPayload(
                    current_time=self.get_datetime().isoformat(),
                    interval=1,
                    status=RegistrationStatus.rejected
                )

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.get_conf_task = asyncio.create_task(self.call(call.GetConfigurationPayload()))

        _, c = run_test(TestCP, 10, speedup=100)

        self.assertTrue(c.get_conf_task.done())
        assert_no_other_packets(self, c)

    #[...]Pending[...] The Central System MAY send request messages to retrieve information from the Charge Point or change its configuration. The Charge Point SHOULD respond to these messages.
    def test_respond_while_pending(self):
        class TestCP(default_central.DefaultChargePoint):
            get_conf_task = None
            first = True

            @on(Action.BootNotification)
            def on_boot_notification(self, charge_point_vendor: str, charge_point_model: str, **kwargs):
                return call_result.BootNotificationPayload(
                    current_time=self.get_datetime().isoformat(),
                    interval=1,
                    status=RegistrationStatus.pending
                )

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                if self.first:
                    self.get_conf_task = asyncio.create_task(self.call(call.GetConfigurationPayload()))
                self.first = False

        _, c = run_test(TestCP, 10, speedup=100)

        self.assertTrue(c.get_conf_task.done())

        self.assertEqual(len(c.received_results), 1, "Expected received get configuration result")
        self.assertEqual(c.received_results[call_result.GetConfigurationPayload], 1, "Expected one get configuration result")

        self.assertEqual(len(c.received_calls), 1, "Expected no other received calls")
        self.assertEqual(len(c.received_errors), 0, "Expected no received errors")

    """
    The Charge
    Point SHALL NOT send request messages to the Central System unless it has been instructed by the Central
    System to do so with a TriggerMessage.req request.
    """
    @unittest.skip("TriggerMessage not implemented yet")
    def test_handle_trigger_message_while_pending(self):
        pass

    """
    The Charge Point SHALL send a BootNotification.req PDU each time it boots or reboots. Between the physical
    power-on/reboot and the successful completion of a BootNotification, where Central System returns Accepted or
    Pending, the Charge Point SHALL NOT send any other request to the Central System. This includes cached
    messages that are still present in the Charge Point from before.
    """
    @unittest.skip("Not implemented yet")
    def test_boot_notification_after_reboot(self):
        pass


    """
    When the Central System responds with a BootNotification.conf with a status Accepted, the Charge Point will
    adjust the heartbeat interval in accordance with the interval from the response PDU
    """
    def test_heartbeat_interval_adjusted(self):
        class TestCP(default_central.DefaultChargePoint):
            get_conf_task = None
            first = True

            @on(Action.BootNotification)
            def on_boot_notification(self, charge_point_vendor: str, charge_point_model: str, **kwargs):
                return call_result.BootNotificationPayload(
                    current_time=self.get_datetime().isoformat(),
                    interval=12321,
                    status=RegistrationStatus.accepted
                )

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                if self.first:
                    self.get_conf_task = asyncio.create_task(self.call(call.GetConfigurationPayload(["HeartbeatInterval"])))
                self.first = False

        _, c = run_test(TestCP, 10, speedup=100)
        self.assertTrue(c.get_conf_task.done())

        result = c.get_conf_task.result()

        self.assertEqual(int(result.configuration_key[0]["value"]), 12321)

    """
    When the Central System responds with a BootNotification.conf with a status Accepted, [...] it is RECOMMENDED to
    synchronize its internal clock with the supplied Central System’s current time.
    """
    def test_clock_adjusted(self):
        class TestCP(default_central.DefaultChargePoint):
            sent_time = 0

            @on(Action.BootNotification)
            def on_boot_notification(self, charge_point_vendor: str, charge_point_model: str, **kwargs):
                t = self.get_datetime()
                self.sent_time = int(t.timestamp())

                return call_result.BootNotificationPayload(
                    current_time=t.isoformat(),
                    # We have to set a high interval here or else the charge point will send heartbeats.
                    # The responses to those contain the ystem time that overrides the one sent here.
                    interval=100,
                    status=RegistrationStatus.accepted
                )

        start = time.time()
        _, c = run_test(TestCP, 10, speedup=100)
        self.assertGreaterEqual(default_platform.last_time_set_at, start)
        self.assertEqual(default_platform.last_time_set, c.sent_time)

    """
    While in pending state, the following Central System initiated messages are not allowed:
    RemoteStartTransaction.req and RemoteStopTransaction.req

    [
        It is unclear what to do if we still receive one from the central. We handle thas as if we've received any message while rejected: Just throw it away and don't answer at all.
    ]
    """
    @unittest.skip("RemoteStartTransaction not implemented yet")
    def test_remote_start_stop_while_pending(self):
        class TestCP(default_central.DefaultChargePoint):
            remote_start_task = None
            first = True

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                if self.first:
                    self.remote_start_task = asyncio.create_task(self.call(call.RemoteStartTransactionPayload("00:11:22:33")))
                self.first = False

        _, c = run_test(TestCP, 10, speedup=100)

        result = c.remote_start_task.result()
        self.assertEqual(result, None)
        assert_no_other_packets(self, c)


if __name__ == '__main__':
    unittest.main()
