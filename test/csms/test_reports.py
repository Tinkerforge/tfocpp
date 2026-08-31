# B07/B08 device model reporting and the CALLRESULTERROR RPC flow
# against the scripted mini CSMS.

import time

import pytest

from minicsms import MiniCsms


@pytest.fixture
def csms():
    c = MiniCsms()
    yield c
    c.stop()


@pytest.fixture
def host(csms, hosts):
    h = hosts.start(csms.url, "tfocpp-report-test")
    csms.wait_connected()
    h.wait_for("Boot notification accepted", timeout=20)
    return h


def collect_report(csms, request_id):
    entries = []
    seq_no = 0
    while True:
        payload, msg_id = csms.expect("NotifyReport")
        csms.respond(msg_id, {})
        assert payload["requestId"] == request_id  # B07.FR.04
        assert payload["seqNo"] == seq_no  # B07.FR.10
        assert "generatedAt" in payload
        seq_no += 1
        entries.extend(payload.get("reportData", []))
        if not payload.get("tbc", False):
            return entries


def by_key(entries):
    return {(e["component"]["name"], e["variable"]["name"], e["variable"].get("instance")): e
            for e in entries}


def test_full_inventory(csms, host):
    resp = csms.call("GetBaseReport", {"requestId": 17, "reportBase": "FullInventory"})
    assert resp["status"] == "Accepted"

    entries = collect_report(csms, 17)
    report = by_key(entries)

    for e in entries:
        # B07.FR.08/B08.FR.12: characteristics for every variable.
        c = e["variableCharacteristics"]
        assert c["dataType"]
        assert c["supportsMonitoring"] is False
        assert len(e["variableAttribute"]) == 1
        assert e["variableAttribute"][0].get("type", "Actual") == "Actual"

    # A00.FR.304: WriteOnly BasicAuthPassword, no value, maxLimit at least 40.
    bap = report[("SecurityCtrlr", "BasicAuthPassword", None)]
    attr = bap["variableAttribute"][0]
    assert attr["mutability"] == "WriteOnly"
    assert "value" not in attr  # B07.FR.03
    assert 40 <= bap["variableCharacteristics"]["maxLimit"] <= 64

    # HUB20-411-005..008: CertificateEntries.maxLimit covers 30+50+40 roots.
    ce = report[("SecurityCtrlr", "CertificateEntries", None)]
    assert ce["variableAttribute"][0]["mutability"] == "ReadOnly"
    assert ce["variableAttribute"][0]["value"] == "0"
    assert ce["variableCharacteristics"]["maxLimit"] >= 120
    assert ce["variableCharacteristics"]["dataType"] == "integer"

    # Instanced variable.
    ma = report[("OCPPCommCtrlr", "MessageAttempts", "TransactionEvent")]
    assert ma["variableAttribute"][0]["value"] == "3"

    hb = report[("OCPPCommCtrlr", "HeartbeatInterval", None)]
    assert hb["variableAttribute"][0]["value"] == "300"
    assert hb["variableCharacteristics"]["unit"] == "s"

    # ReadOnly variables are included in the full inventory.
    assert ("SecurityCtrlr", "SecurityProfile", None) in report
    assert ("DeviceDataCtrlr", "ItemsPerMessage", "GetReport") in report
    assert ("DeviceDataCtrlr", "BytesPerMessage", "GetVariables") in report

    suite = report[("ISO15118Ctrlr", "V2G20SECCLeafCryptoSuite", None)]
    assert suite["variableCharacteristics"]["dataType"] == "OptionList"
    assert suite["variableCharacteristics"]["valuesList"] == "ecdsa_secp521r1_sha512,ed448"


def test_v2g20_crypto_suite_option_list(csms, host):
    variable = {
        "component": {"name": "ISO15118Ctrlr"},
        "variable": {"name": "V2G20SECCLeafCryptoSuite"},
    }

    for value in ("ed448", "ecdsa_secp521r1_sha512,ed448", "ecdsa_secp521r1_sha512"):
        response = csms.call("SetVariables", {
            "setVariableData": [{**variable, "attributeValue": value}],
        })
        assert response["setVariableResult"][0]["attributeStatus"] == "Accepted"

        response = csms.call("GetVariables", {"getVariableData": [variable]})
        assert response["getVariableResult"][0]["attributeValue"] == value

    response = csms.call("SetVariables", {
        "setVariableData": [{**variable, "attributeValue": ""}],
    })
    assert response["setVariableResult"][0]["attributeStatus"] == "Rejected"


def test_configuration_inventory(csms, host):
    resp = csms.call("GetBaseReport", {"requestId": 18, "reportBase": "ConfigurationInventory"})
    assert resp["status"] == "Accepted"

    entries = collect_report(csms, 18)
    report = by_key(entries)

    # B07.FR.07: only variables that can be set by the operator.
    for e in entries:
        assert e["variableAttribute"][0]["mutability"] in ("ReadWrite", "WriteOnly")

    assert ("OCPPCommCtrlr", "HeartbeatInterval", None) in report
    assert ("SecurityCtrlr", "BasicAuthPassword", None) in report
    assert ("SecurityCtrlr", "SecurityProfile", None) not in report
    assert ("DeviceDataCtrlr", "ItemsPerMessage", "GetReport") not in report


def test_summary_inventory_not_supported(csms, host):
    # B07.FR.02/12: SummaryInventory is optional and not implemented.
    resp = csms.call("GetBaseReport", {"requestId": 19, "reportBase": "SummaryInventory"})
    assert resp["status"] == "NotSupported"

    # B07.FR.05: no NotifyReport follows.
    with pytest.raises(TimeoutError):
        csms.expect("NotifyReport", timeout=3)


def test_get_report_component_filter(csms, host):
    resp = csms.call("GetReport", {
        "requestId": 20,
        "componentVariable": [{"component": {"name": "ISO15118Ctrlr"}}],
    })
    assert resp["status"] == "Accepted"

    entries = collect_report(csms, 20)
    assert all(e["component"]["name"] == "ISO15118Ctrlr" for e in entries)
    names = {e["variable"]["name"] for e in entries}
    assert {"SeccId", "CountryName", "OrganizationName", "V2G20SECCLeafCryptoSuite",
            "Enabled", "V2GCertificateInstallationEnabled",
            "ContractCertificateInstallationEnabled", "ISO15118EvseId",
            "EnforceTlsEnabled", "PrivateEnviromentEnabled",
            "PWMChargingFallbackTimeout", "ProtocolSupported"} == names


def test_get_report_variable_filter(csms, host):
    # B08.FR.21: a request without the instance matches every instance.
    resp = csms.call("GetReport", {
        "requestId": 21,
        "componentVariable": [{"component": {"name": "OCPPCommCtrlr"},
                               "variable": {"name": "MessageAttempts"}}],
    })
    assert resp["status"] == "Accepted"

    entries = collect_report(csms, 21)
    assert len(entries) == 1
    assert entries[0]["variable"]["instance"] == "TransactionEvent"


def test_get_report_empty_result_set(csms, host):
    # B08.FR.10: no component has a Problem variable set to true.
    resp = csms.call("GetReport", {"requestId": 22, "componentCriteria": ["Problem"]})
    assert resp["status"] == "EmptyResultSet"

    resp = csms.call("GetReport", {
        "requestId": 23,
        "componentVariable": [{"component": {"name": "NoSuchCtrlr"}}],
    })
    assert resp["status"] == "EmptyResultSet"

    with pytest.raises(TimeoutError):
        csms.expect("NotifyReport", timeout=3)


def test_get_report_unfiltered(csms, host):
    # B08: componentVariable and componentCriteria absent reports everything.
    resp = csms.call("GetReport", {"requestId": 24})
    assert resp["status"] == "Accepted"

    full = csms.call("GetBaseReport", {"requestId": 25, "reportBase": "FullInventory"})
    # B08.FR.16/B07.FR.13: a second report while one is streaming.
    assert full["status"] == "Rejected"

    entries = collect_report(csms, 24)

    resp = csms.call("GetBaseReport", {"requestId": 26, "reportBase": "FullInventory"})
    assert resp["status"] == "Accepted"
    assert len(collect_report(csms, 26)) == len(entries)


def test_get_variables_with_instance(csms, host):
    resp = csms.call("GetVariables", {"getVariableData": [
        {"component": {"name": "DeviceDataCtrlr"},
         "variable": {"name": "ItemsPerMessage", "instance": "GetReport"}},
        {"component": {"name": "DeviceDataCtrlr"},
         "variable": {"name": "ItemsPerMessage", "instance": "NoSuchMessage"}},
    ]})
    results = resp["getVariableResult"]
    assert results[0]["attributeStatus"] == "Accepted"
    assert results[0]["attributeValue"] == "16"
    assert results[1]["attributeStatus"] == "UnknownVariable"


def test_call_result_error(csms, host):
    # FR.06: an invalid CALLRESULT is answered with a CALLRESULTERROR.
    csms.mangle_next_heartbeat = True
    assert csms.call("TriggerMessage", {"requestedMessage": "Heartbeat"})["status"] == "Accepted"

    host.wait_for("Sending call result error", timeout=15)

    deadline = time.monotonic() + 10
    while time.monotonic() < deadline and not csms.result_errors:
        time.sleep(0.2)
    assert csms.result_errors
    msg_id, code, _desc = csms.result_errors[0]
    assert msg_id == csms.mangled_ids[0]
    assert code == "TypeConstraintViolation"

    # The connection stays usable.
    assert csms.call("GetVariables", {"getVariableData": [
        {"component": {"name": "OCPPCommCtrlr"}, "variable": {"name": "HeartbeatInterval"}},
    ]})["getVariableResult"][0]["attributeStatus"] == "Accepted"
