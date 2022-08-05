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

class TestAuthorize(unittest.TestCase):
    """
    the Charge Point SHALL send an Authorize.req PDU to the Central System to request authorization
    """
    def test_authorize_sent_on_tag(test):
        # We don't test for local auth list and auth cache behaviour, only the core profile is supported for now.
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.Authorize)
            def after_authorize(self, id_tag):
                test.assertEqual(id_tag, "C0:FF:EE")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    The Charge Point SHALL only supply energy after authorization.
    """
    def test_authorization_status(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            state = 0
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @on(Action.Authorize)
            def on_authorize(self, id_tag):
                status = {
                    0: "ConcurrentTx",
                    1: "Invalid",
                    2: "Expired",
                    3: "Blocked",
                    4: "Accepted"
                }[self.state]

                self.state += 1

                return call_result.AuthorizePayload(id_tag_info={"status": status})

            @after(Action.Authorize)
            def after_authorize(self, id_tag):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                self.done = self.state == 5

        def verify_current(libocpp, c):
            if c is None or c.state != 5:
                if 1 in default_platform.charging_current:
                    test.assertEqual(default_platform.charging_current[1], 0)

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100, tick_fn=verify_current)
        test.assertTrue(c.done)

    def test_reauthorize_other_tag_to_stop(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            first = True
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED

            @after(Action.Authorize)
            def after_authorize(self, id_tag):
                if self.first:
                    test.assertEqual(id_tag, "C0:FF:EE")
                    self.first = False
                else:
                    test.assertEqual(id_tag, "CA:FE")
                    self.done = True

            @after(Action.StatusNotification)
            def after_sn(self, connector_id, error_code, status, **kwargs):
                if status == "SuspendedEVSE":
                    default_platform.show_tag(test, 1, "CA:FE")


        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
        test.assertEqual(c.received_calls[Action.Authorize], 2)

    def test_dont_reauthorize_on_same_tag(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")
                default_platform.connector_state[1] = default_platform.CONNECTOR_STATE_CONNECTED

            @after(Action.Authorize)
            def after_authorize(self, id_tag):
                test.assertEqual(id_tag, "C0:FF:EE")

            @after(Action.StatusNotification)
            def after_sn(self, connector_id, error_code, status, **kwargs):
                if connector_id != 1:
                    return
                if status == "SuspendedEVSE":
                    default_platform.show_tag(test, 1, "C0:FF:EE")
                if status == "Finishing":
                    self.done = True

        _, c = run_test(TestCP, sim_len_secs=10, speedup=100)
        test.assertTrue(c.done)
        test.assertEqual(c.received_calls[Action.Authorize], 1)
        test.assertEqual(c.status[1], ["Available", "Preparing", "SuspendedEVSE", "Finishing"])
