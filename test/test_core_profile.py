from datetime import datetime
import unittest
import asyncio
import time
import itertools

from ocpp.routing import on, after
from ocpp.v16.enums import Action, RegistrationStatus
from ocpp.v16 import call_result, call
import ocpp.exceptions

import default_platform
import default_central

from test_runner import run_test

measurands = [
    "Current.Export",
    "Current.Import",
    "Current.Offered",
    "Energy.Active.Export.Register",
    "Energy.Active.Import.Register",
    "Energy.Reactive.Export.Register",
    "Energy.Reactive.Import.Register",
    "Energy.Active.Export.Interval",
    "Energy.Active.Import.Interval",
    "Energy.Reactive.Export.Interval",
    "Energy.Reactive.Import.Interval",
    "Frequency",
    "Power.Active.Export",
    "Power.Active.Import",
    "Power.Factor",
    "Power.Offered",
    "Power.Reactive.Export",
    "Power.Reactive.Import",
    "RPM",
    "SoC",
    "Temperature",
    "Voltage",
]

phases = [
    "L1",
    "L2",
    "L3",
    "N",
    "L1-N",
    "L2-N",
    "L3-N",
    "L1-L2",
    "L2-L3",
    "L3-L1"
]

phase_rotations = ["NotApplicable", "Unknown", "RST", "RTS", "SRT", "STR", "TRS", "TSR"]

feature_profiles = [
    "Core",
    "FirmwareManagement",
    "LocalAuthListManagement",
    "Reservation",
    "SmartCharging",
    "RemoteTrigger",
]

expected_config = [
    ("AllowOfflineTxForUnknownId",        "optional", "RW",   "boolean"),  #9.1.1.
    ("AuthorizationCacheEnabled",         "optional", "RW",   "boolean"),  #9.1.2.
    ("AuthorizeRemoteTxRequests",         "required", "R/RW", "boolean"),  #9.1.3. R or RW. Choice is up to Charge Point implementation.
    ("BlinkRepeat",                       "optional", "RW",   "integer"),  #9.1.4.
    ("ClockAlignedDataInterval",          "required", "RW",   "integer"),  #9.1.5.
    ("ConnectionTimeOut",                 "required", "RW",   "integer"),  #9.1.6.
    ("ConnectorPhaseRotation",            "required", "RW",   "numCSL", phase_rotations),   #9.1.7. numCSL for CSL with indices beforehand
    ("ConnectorPhaseRotationMaxLength",   "optional", "R",    "integer"),  #9.1.8.
    ("GetConfigurationMaxKeys",           "required", "R",    "integer"),  #9.1.9.
    ("HeartbeatInterval",                 "required", "RW",   "integer"),  #9.1.10.
    ("LightIntensity",                    "optional", "RW",   "integer"),  #9.1.11.
    ("LocalAuthorizeOffline",             "required", "RW",   "boolean"),  #9.1.12.
    ("LocalPreAuthorize",                 "required", "RW",   "boolean"),  #9.1.13.
    ("MaxEnergyOnInvalidId",              "optional", "RW",   "integer"),  #9.1.14.
    ("MeterValuesAlignedData",            "required", "RW",   "CSL", measurands),      #9.1.15.
    ("MeterValuesAlignedDataMaxLength",   "optional", "R",    "integer"),  #9.1.16.

    # Where applicable, the Measurand is combined with the optional phase; for instance: Voltage.L1
    ("MeterValuesSampledData",            "required", "RW",   "CSL", [x + y for x in measurands for y in ([""] + [".{}".format(p) for p in phases])]),      #9.1.17.
    ("MeterValuesSampledDataMaxLength",   "optional", "R",    "integer"),  #9.1.18.
    ("MeterValueSampleInterval",          "required", "RW",   "integer"),  #9.1.19.
    ("MinimumStatusDuration",             "optional", "RW",   "integer"),  #9.1.20.
    ("NumberOfConnectors",                "required", "R",    "integer"),  #9.1.21.
    ("ResetRetries",                      "required", "RW",   "integer"),  #9.1.22.
    ("StopTransactionMaxMeterValues",     "optional", "R",    "integer"),  #9.1.23. (Errata 4.0)
    ("StopTransactionOnEVSideDisconnect", "optional", "R/RW", "boolean"),  #9.1.23. (changed to optional R/RW in Errata 4.0)
    ("StopTransactionOnInvalidId",        "required", "RW",   "boolean"),  #9.1.24.
    ("StopTxnAlignedData",                "required", "RW",   "CSL", measurands),      #9.1.25.
    ("StopTxnAlignedDataMaxLength",       "optional", "R",    "integer"),  #9.1.26.
    ("StopTxnSampledData",                "required", "RW",   "CSL", measurands),      #9.1.27.
    ("StopTxnSampledDataMaxLength",       "optional", "R",    "integer"),  #9.1.28.
    ("SupportedFeatureProfiles",          "required", "R",    "CSL", feature_profiles),      #9.1.29.
    ("SupportedFeatureProfilesMaxLength", "optional", "R",    "integer"),  #9.1.30.
    ("TransactionMessageAttempts",        "required", "RW",   "integer"),  #9.1.31.
    ("TransactionMessageRetryInterval",   "required", "RW",   "integer"),  #9.1.32.
    ("UnlockConnectorOnEVSideDisconnect", "required", "RW",   "boolean"),  #9.1.33.
    ("WebSocketPingInterval",             "optional", "RW",   "integer"),  #9.1.34.
]

class TestCoreProfile(unittest.TestCase):
    def test_core_profile_supported(self):
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload([])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        ck =  c.task.result().configuration_key
        val = None
        for conf in ck:
            if conf["key"] != "SupportedFeatureProfiles":
                continue
            val = conf["value"]

        self.assertIn("Core", val)

    def test_get_all_config_values(self):
        class TestCP(default_central.DefaultChargePoint):
            task = None

            @after(Action.BootNotification)
            def after_boot_notification(self, *args, **kwargs):
                self.task = asyncio.create_task(self.call(call.GetConfigurationPayload([])))

        _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

        ck =  c.task.result().configuration_key
        self.assertGreaterEqual(len(ck), 21, "three known keys requested")

        # OCPP does not specify the result array order, so testing this is a bit more complicated
        for name, required, rw, type_, *valid_values in expected_config:
            for config in ck:
                if config["key"] != name:
                    continue

                if rw == "R":
                    self.assertTrue(config["readonly"])
                elif rw == "RW":
                    self.assertFalse(config["readonly"])

                self.assertIsInstance(config["value"], str, "All values must be passed as string")

                if type_ == "integer":
                    try:
                        int(config["value"])
                    except ValueError:
                        self.fail("Key {} must have a value that can be parsed as integer, not '{}'".format(name, config["value"]))
                elif type_ == "boolean":
                    self.assertIn(config["value"], ["true", "false"], name)
                elif type_ == "CSL":
                    if len(config["value"]) == 0:
                        break

                    for x in config["value"].split(","):
                        self.assertIn(x, valid_values[0], name)
                elif type_ == "numCSL":
                    if len(config["value"]) == 0:
                        break

                    zero_found = False
                    for x in config["value"].split(","):
                        num, val = x.split(".")
                        self.assertIn(val, valid_values[0], name)
                        try:
                            num = int(num)
                        except ValueError:
                            self.fail("Key {} must have a value that can be parsed as integer, not '{}'".format(name, config["value"]))
                        if num == 0:
                            zero_found = True

                    offset = 0 if zero_found else 1
                    self.assertEqual(sorted([int(x.split(".")[0]) for x in config["value"].split(",")]), list(range(offset, offset + len(config["value"].split(",")))))
                break
            else:
                if required == "required":
                    self.fail("Required configuration key {} not found in response.".format(name))

    # This is not specified, however negative values don't make any sense for
    # all integer configuration keys. We check that the default value is not
    # negative and it is not possible to set them to a negative value.
    # (Only WebSocketPingInterval specifies that negative values are not allowed)
    def test_non_negative(self):
        for name, required, rw, type_, *valid_values in expected_config:
            if type_ != "integer":
                continue

            class TestCP(default_central.DefaultChargePoint):
                @after(Action.BootNotification)
                async def after_boot_notification(inner_self, *args, **kwargs):
                    result = await inner_self.call(call.GetConfigurationPayload([name]))

                    if (len(result.configuration_key) == 0 and len(result.unknown_key) == 1):
                        inner_self.done = True
                        return

                    conf = result.configuration_key[0]

                    self.assertEqual(conf["key"], name)
                    self.assertGreaterEqual(int(conf["value"]), 0)

                    if conf["readonly"]:
                        inner_self.done = True
                        return

                    await inner_self.call(call.ChangeConfigurationPayload(name, '-1'))
                    new_conf = (await inner_self.call(call.GetConfigurationPayload([name]))).configuration_key[0]

                    self.assertEqual(conf["value"], new_conf["value"])
                    inner_self.done = True

            _, c = run_test(TestCP, sim_len_secs=2, speedup=100)

            self.assertTrue(c.done) # If this fails one of the asserts in the async code has probably failed.
