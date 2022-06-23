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
    When stopping a Transaction, the
    Charge Point SHALL only send an Authorize.req when the identifier used for stopping the transaction is different
    from the identifier that started the transaction.
    """
    def test_reauthorize_on_other_tag(test):
        #TODO: start transaction to be able to stop it with another tag
        class TestCP(default_central.DefaultChargePoint):
            first = True
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.Authorize)
            def after_authorize(self, id_tag):
                if self.first:
                    test.assertEqual(id_tag, "C0:FF:EE")
                    default_platform.show_tag(test, 1, "CA:FE")
                    self.first = False
                else:
                    default_platform.show_tag(test, 1, "CA:FE")
                    self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    def test_dont_reauthorize_on_same_tag(test):
        #TODO: use status notifications to make sure we are in idle again
        class TestCP(default_central.DefaultChargePoint):
            first = True
            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                default_platform.show_tag(test, 1, "C0:FF:EE")

            @after(Action.Authorize)
            def after_authorize(self, id_tag):
                test.assertEqual(id_tag, "C0:FF:EE")
                if self.first:
                    default_platform.show_tag(test, 1, "C0:FF:EE")
                    self.first = False


        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        #test.assertTrue(c.done)
        test.assertEqual(c.received_calls[Action.Authorize], 1)
