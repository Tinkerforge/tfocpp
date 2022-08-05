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

class TestChangeConfiguration(unittest.TestCase):
    def test_bool(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                req = await self.call(call.ChangeConfigurationPayload("LocalAuthorizeOffline", "true"))
                test.assertEqual(req.status, "Accepted")

                conf = (await self.call(call.GetConfigurationPayload(["LocalAuthorizeOffline"]))).configuration_key[0]
                test.assertEqual(conf["key"], "LocalAuthorizeOffline")
                test.assertEqual(conf["value"], "true")

                req = await self.call(call.ChangeConfigurationPayload("LocalAuthorizeOffline", "false"))
                test.assertEqual(req.status, "Accepted")

                conf = (await self.call(call.GetConfigurationPayload(["LocalAuthorizeOffline"]))).configuration_key[0]
                test.assertEqual(conf["key"], "LocalAuthorizeOffline")
                test.assertEqual(conf["value"], "false")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    def test_integer(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                req = await self.call(call.ChangeConfigurationPayload("ConnectionTimeOut", "123"))
                test.assertEqual(req.status, "Accepted")

                conf = (await self.call(call.GetConfigurationPayload(["ConnectionTimeOut"]))).configuration_key[0]
                test.assertEqual(conf["key"], "ConnectionTimeOut")
                test.assertEqual(conf["value"], "123")

                req = await self.call(call.ChangeConfigurationPayload("ConnectionTimeOut", "0"))
                test.assertEqual(req.status, "Accepted")

                conf = (await self.call(call.GetConfigurationPayload(["ConnectionTimeOut"]))).configuration_key[0]
                test.assertEqual(conf["key"], "ConnectionTimeOut")
                test.assertEqual(conf["value"], "0")
                self.done = True

                req = await self.call(call.ChangeConfigurationPayload("ConnectionTimeOut", "000456"))
                test.assertEqual(req.status, "Accepted")

                conf = (await self.call(call.GetConfigurationPayload(["ConnectionTimeOut"]))).configuration_key[0]
                test.assertEqual(conf["key"], "ConnectionTimeOut")
                test.assertEqual(conf["value"], "456")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    def test_csl(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", "Current.Export"))
                test.assertEqual(req.status, "Accepted")

                conf = (await self.call(call.GetConfigurationPayload(["MeterValuesAlignedData"]))).configuration_key[0]
                test.assertEqual(conf["key"], "MeterValuesAlignedData")
                test.assertEqual(conf["value"], "Current.Export")

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", "Current.Offered,Frequency,Energy.Reactive.Import.Interval,SoC"))
                test.assertEqual(req.status, "Accepted")

                conf = (await self.call(call.GetConfigurationPayload(["MeterValuesAlignedData"]))).configuration_key[0]
                test.assertEqual(conf["key"], "MeterValuesAlignedData")
                test.assertEqual(conf["value"], "Current.Offered,Frequency,Energy.Reactive.Import.Interval,SoC")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    def test_numcsl(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "0.RST,1.NotApplicable"))
                test.assertEqual(req.status, "Accepted")

                conf = (await self.call(call.GetConfigurationPayload(["ConnectorPhaseRotation"]))).configuration_key[0]
                test.assertEqual(conf["key"], "ConnectorPhaseRotation")
                test.assertEqual(conf["value"], "0.RST,1.NotApplicable")

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "1.RST,0.SRT"))
                test.assertEqual(req.status, "Accepted")

                conf = (await self.call(call.GetConfigurationPayload(["ConnectorPhaseRotation"]))).configuration_key[0]
                test.assertEqual(conf["key"], "ConnectorPhaseRotation")
                test.assertEqual(conf["value"], "1.RST,0.SRT")

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "1.RST,3.SRT"))
                test.assertEqual(req.status, "Accepted")

                conf = (await self.call(call.GetConfigurationPayload(["ConnectorPhaseRotation"]))).configuration_key[0]
                test.assertEqual(conf["key"], "ConnectorPhaseRotation")
                test.assertEqual(conf["value"], "1.RST,3.SRT")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    If "key" does not correspond to a configuration setting supported by Charge Point, it SHALL respond with
    a status 'NotSupported'.
    """
    def test_unknown_key(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                req = await self.call(call.ChangeConfigurationPayload("UnknownKey1234", "Current.Export"))
                test.assertEqual(req.status, "NotSupported")
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    """
    'Rejected' are requests with out-of-range values and
    requests with values that do not conform to an expected format.
    """
    def test_out_of_range(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                expected = []
                got = []

                req = await self.call(call.ChangeConfigurationPayload("WebSocketPingInterval", "-3"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("LocalAuthorizeOffline", "notabool"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", "Current.Offered,Frequency,Energy.Reactive.Import.Interval,SoC,ThisKeyDoesNotExist"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "1.RST,2.XYZ"))
                expected.append("Rejected")
                got.append(req.status)

                num_connectors = int((await self.call(call.GetConfigurationPayload(["NumberOfConnectors"]))).configuration_key[0]["value"])

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", ",".join("{}.RST".format(i) for i in range(0, num_connectors + 2))))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "-1.RST,0.RST"))
                expected.append("Rejected")
                got.append(req.status)

                test.assertEqual(expected, got)
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    def test_csl_separator_errors(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                expected = []
                got = []

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", "Current.Offered,,,Energy"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", "Current.Offered,Energy,"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", "Current.Offered,Energy,,,"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", ",Current.Offered"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", ",,,Current.Offered"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", ","))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", ",,,"))
                expected.append("Rejected")
                got.append(req.status)

                test.assertEqual(expected, got)
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=3, speedup=100)
        test.assertTrue(c.done)

    def test_numcsl_separator_errors(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                expected = []
                got = []

                # , errors

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "0.RST,,,1.RST"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "0.RST,1.RST,"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "0.RST,1.RST,,,"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", ",0.RST"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", ",,,0.RST"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", ","))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", ",,,"))
                expected.append("Rejected")
                got.append(req.status)

                # . errors

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "0.RST,1..RST"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "0.RST,1.1.RST"))
                expected.append("Rejected")
                got.append(req.status)


                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "0.RST,1.RST."))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "0.RST,1.RST..."))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", ".0.RST"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "...0.RST"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "."))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "..."))
                expected.append("Rejected")
                got.append(req.status)

                # , and . errors

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "0.RST,1.,RST"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", "0.RST,1,.RST"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", ",."))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectorPhaseRotation", ".,"))
                expected.append("Rejected")
                got.append(req.status)

                test.assertEqual(expected, got)
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=3, speedup=100)
        test.assertTrue(c.done)

    def test_empty_value(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                expected = []
                got = []

                req = await self.call(call.ChangeConfigurationPayload("WebSocketPingInterval", ""))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("LocalAuthorizeOffline", ""))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", ""))
                expected.append("Accepted")
                got.append(req.status)

                test.assertEqual(expected, got)
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    def test_type_error_int(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                expected = []
                got = []

                req = await self.call(call.ChangeConfigurationPayload("ConnectionTimeOut", "0.ABC"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectionTimeOut", "123AB"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectionTimeOut", "0x123"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectionTimeOut", "false"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectionTimeOut", "true"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectionTimeOut", "Current.Offered"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("ConnectionTimeOut", "Current.Offered,RPM"))
                expected.append("Rejected")
                got.append(req.status)

                test.assertEqual(expected, got)
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=3, speedup=100)
        test.assertTrue(c.done)

    def test_type_error_bool(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                expected = []
                got = []

                req = await self.call(call.ChangeConfigurationPayload("LocalAuthorizeOffline", "-1"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("LocalAuthorizeOffline", "0"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("LocalAuthorizeOffline", "1"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("LocalAuthorizeOffline", "12"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("LocalAuthorizeOffline", "Current.Offered"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("LocalAuthorizeOffline", "Current.Offered,RPM"))
                expected.append("Rejected")
                got.append(req.status)

                test.assertEqual(expected, got)
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)

    def test_type_error_csl(test):
        @default_central.addTester(test)
        class TestCP(default_central.DefaultChargePoint):
            @after(Action.BootNotification)
            async def after_boot_notification(self, *args, **kwargs):
                expected = []
                got = []

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", "-1"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", "12"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", "true"))
                expected.append("Rejected")
                got.append(req.status)

                req = await self.call(call.ChangeConfigurationPayload("MeterValuesAlignedData", "false"))
                expected.append("Rejected")
                got.append(req.status)

                test.assertEqual(expected, got)
                self.done = True

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)
        test.assertTrue(c.done)
