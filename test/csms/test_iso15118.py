# ISO15118Ctrlr variables, the combined certificate flow and the
# M01/M02 contract certificate plumbing against the scripted mini CSMS.

import pytest

from minicsms import MiniCsms
from testca import SigningCa


@pytest.fixture
def ca(tmp_path):
    d = tmp_path / "ca"
    d.mkdir()
    return SigningCa(d)


@pytest.fixture
def csms(ca):
    cert, key = ca.server_cert()
    c = MiniCsms(certfile=cert, keyfile=key)
    yield c
    c.stop()


def start_host(hosts, csms, name, ca):
    return hosts.start(csms.url, name, password="tfocpp-iso-test-password", ca=str(ca.cert))


@pytest.fixture
def host(csms, hosts, ca):
    h = start_host(hosts, csms, "tfocpp-iso-test", ca)
    csms.wait_connected()
    h.wait_for("Boot notification accepted", timeout=20)
    return h


def get_variables(csms, *names):
    data = []
    for name in names:
        if isinstance(name, tuple):
            data.append({"component": {"name": "ISO15118Ctrlr"},
                         "variable": {"name": name[0], "instance": name[1]}})
        else:
            data.append({"component": {"name": "ISO15118Ctrlr"}, "variable": {"name": name}})
    return csms.call("GetVariables", {"getVariableData": data})["getVariableResult"]


def set_variable(csms, name, value):
    res = csms.call("SetVariables", {"setVariableData": [{
        "component": {"name": "ISO15118Ctrlr"},
        "variable": {"name": name},
        "attributeValue": value,
    }]})
    return res["setVariableResult"][0]["attributeStatus"]


def test_iso_variable_defaults(csms, host):
    results = get_variables(csms, "Enabled", "V2GCertificateInstallationEnabled",
                            "ContractCertificateInstallationEnabled", "ISO15118EvseId",
                            "EnforceTlsEnabled", "PrivateEnviromentEnabled",
                            "PWMChargingFallbackTimeout")
    values = [r["attributeValue"] for r in results]
    assert all(r["attributeStatus"] == "Accepted" for r in results)
    assert values == ["true", "true", "true", "ZZ00000", "false", "false", "7"]


def test_iso_variable_validation_and_persistence(csms, hosts, ca):
    name = "tfocpp-iso-persist"
    h = start_host(hosts, csms, name, ca)
    csms.wait_connected()
    h.wait_for("Boot notification accepted", timeout=20)

    # minLimit 7 and maxLimit 37 for the EvseId.
    assert set_variable(csms, "ISO15118EvseId", "ZZ0000") == "Rejected"
    assert set_variable(csms, "ISO15118EvseId", "Z" * 38) == "Rejected"
    assert set_variable(csms, "ISO15118EvseId", "DE*ICE*E*1234567890*1") == "Accepted"
    assert set_variable(csms, "PWMChargingFallbackTimeout", "0") == "Rejected"
    assert set_variable(csms, "PWMChargingFallbackTimeout", "15") == "Accepted"
    assert set_variable(csms, "EnforceTlsEnabled", "true") == "Accepted"
    assert set_variable(csms, "PrivateEnviromentEnabled", "maybe") == "Rejected"
    assert set_variable(csms, "Enabled", "false") == "Accepted"

    h.stop()
    h2 = start_host(hosts, csms, name, ca)
    csms.wait_connected()
    h2.wait_for("Boot notification accepted", timeout=20)

    results = get_variables(csms, "ISO15118EvseId", "PWMChargingFallbackTimeout",
                            "EnforceTlsEnabled", "Enabled")
    assert [r["attributeValue"] for r in results] == ["DE*ICE*E*1234567890*1", "15", "true", "false"]


def test_protocol_supported_instances(csms, host):
    results = get_variables(csms, ("ProtocolSupported", "1"), ("ProtocolSupported", "2"),
                            ("ProtocolSupported", "3"))
    assert results[0]["attributeStatus"] == "Accepted"
    assert results[0]["attributeValue"] == "urn:iso:15118:2:2013:MsgDef,2,0"
    assert results[1]["attributeStatus"] == "Accepted"
    assert results[1]["attributeValue"] == "urn:iso:std:iso:15118:-20:AC,1,0"
    # An unset instance is absent.
    assert results[2]["attributeStatus"] == "UnknownVariable"

    # Read only.
    res = csms.call("SetVariables", {"setVariableData": [{
        "component": {"name": "ISO15118Ctrlr"},
        "variable": {"name": "ProtocolSupported", "instance": "1"},
        "attributeValue": "urn:example,1,0",
    }]})
    assert res["setVariableResult"][0]["attributeStatus"] == "Rejected"

    # B08: only the set instances are reported.
    resp = csms.call("GetReport", {"requestId": 31, "componentVariable": [
        {"component": {"name": "ISO15118Ctrlr"}, "variable": {"name": "ProtocolSupported"}},
    ]})
    assert resp["status"] == "Accepted"
    payload, msg_id = csms.expect("NotifyReport")
    csms.respond(msg_id, {})
    instances = sorted(e["variable"].get("instance") for e in payload["reportData"])
    assert instances == ["1", "2"]


def test_v2g_certificate_installation_disabled(csms, host):
    # V2GCertificateInstallationEnabled gates A02/A03 for the V2G types.
    assert set_variable(csms, "V2GCertificateInstallationEnabled", "false") == "Accepted"
    for trigger in ["SignV2GCertificate", "SignV2G20Certificate", "SignCombinedCertificate"]:
        assert csms.call("TriggerMessage", {"requestedMessage": trigger})["status"] == "Rejected"
    assert csms.call("TriggerMessage", {
        "requestedMessage": "SignChargingStationCertificate",
    })["status"] == "Accepted"
    sign_req, msg_id = csms.expect("SignCertificate")
    assert sign_req["certificateType"] == "ChargingStationCertificate"
    csms.respond(msg_id, {"status": "Rejected"})

    assert set_variable(csms, "V2GCertificateInstallationEnabled", "true") == "Accepted"
    assert csms.call("TriggerMessage", {
        "requestedMessage": "SignV2GCertificate",
    })["status"] == "Accepted"
    sign_req, msg_id = csms.expect("SignCertificate")
    assert sign_req["certificateType"] == "V2GCertificate"
    csms.respond(msg_id, {"status": "Rejected"})


def test_sign_combined_certificate(csms, host, ca):
    # A combined certificate serves the CSMS connection and ISO 15118,
    # certificateType is omitted in both directions.
    assert csms.call("InstallCertificate", {
        "certificateType": "V2GRootCertificate",
        "certificate": ca.cert_pem,
    })["status"] == "Accepted"

    assert csms.call("TriggerMessage", {
        "requestedMessage": "SignCombinedCertificate",
    })["status"] == "Accepted"

    sign_req, msg_id = csms.expect("SignCertificate")
    assert "certificateType" not in sign_req
    csms.respond(msg_id, {"status": "Accepted"})

    # The V2G PKI dictates the subject, CN is the SeccId.
    from cryptography import x509
    from cryptography.x509.oid import NameOID
    csr = x509.load_pem_x509_csr(sign_req["csr"].encode())
    cn = csr.subject.get_attributes_for_oid(NameOID.COMMON_NAME)[0].value
    assert cn == "DE*TFO*E000001"

    leaf = ca.sign_csr(sign_req["csr"])
    assert csms.call("CertificateSigned", {
        "certificateChain": leaf,
        "requestId": sign_req["requestId"],
    })["status"] == "Accepted"
    host.wait_for("Installed the signed CombinedCertificate", timeout=10)

    # The chain is listed as a V2G certificate chain (M03).
    listed = csms.call("GetInstalledCertificateIds", {"certificateType": ["V2GCertificateChain"]})
    assert listed["status"] == "Accepted"
    assert len(listed["certificateHashDataChain"]) == 1


def test_get_15118_ev_certificate(csms, host):
    # M01: a CertificateInstallationReq is forwarded and the response
    # handed back to the ISO 15118 stack.
    host.send("evcert")
    payload, msg_id = csms.expect("Get15118EVCertificate")
    assert payload["action"] == "Install"
    assert payload["iso15118SchemaVersion"] == "urn:iso:15118:2:2013:MsgDef"
    assert payload["exiRequest"] == "3q2+7w=="
    # M01.FR.02: no maximumContractCertificateChains for ISO 15118-2.
    assert "maximumContractCertificateChains" not in payload
    csms.respond(msg_id, {"status": "Accepted", "exiResponse": "ZXhpCg=="})
    host.wait_for("EV certificate result: Accepted, 0 remaining, exi ZXhpCg==", timeout=10)

    # M02: an update request with remaining contracts (M01.FR.05).
    host.send("evcert update")
    payload, msg_id = csms.expect("Get15118EVCertificate")
    assert payload["action"] == "Update"
    csms.respond(msg_id, {"status": "Accepted", "exiResponse": "ZXhpCg==", "remainingContracts": 2})
    host.wait_for("EV certificate result: Accepted, 2 remaining", timeout=10)

    # A failed response reports no EXI data.
    host.send("evcert")
    payload, msg_id = csms.expect("Get15118EVCertificate")
    csms.respond(msg_id, {"status": "Failed", "exiResponse": "ZXhpCg=="})
    host.wait_for("EV certificate result: Failed, 0 remaining, exi -", timeout=10)


def test_get_15118_ev_certificate_disabled(csms, host):
    # ContractCertificateInstallationEnabled gates M01/M02.
    assert set_variable(csms, "ContractCertificateInstallationEnabled", "false") == "Accepted"
    host.send("evcert")
    host.wait_for("EV certificate request refused", timeout=10)

    assert set_variable(csms, "ContractCertificateInstallationEnabled", "true") == "Accepted"
    assert set_variable(csms, "Enabled", "false") == "Accepted"
    host.send("evcert")
    host.wait_for("EV certificate request refused", timeout=10)


def test_cert_store_change_notification(csms, host, ca):
    # The registered callback fires on root install, chain install
    # and certificate deletion so the ISO 15118 stack can reload.
    assert csms.call("InstallCertificate", {
        "certificateType": "V2GRootCertificate",
        "certificate": ca.cert_pem,
    })["status"] == "Accepted"
    host.wait_for("Cert store changed: v2g chain 0, v2g20 chain 0, v2g roots 1, oem roots 0, mo roots 0", timeout=10)

    assert csms.call("TriggerMessage", {
        "requestedMessage": "SignV2G20Certificate",
    })["status"] == "Accepted"
    sign_req, msg_id = csms.expect("SignCertificate")
    assert sign_req["certificateType"] == "V2G20Certificate"
    csms.respond(msg_id, {"status": "Accepted"})
    leaf = ca.sign_csr(sign_req["csr"])
    assert csms.call("CertificateSigned", {
        "certificateChain": leaf,
        "certificateType": "V2G20Certificate",
        "requestId": sign_req["requestId"],
    })["status"] == "Accepted"
    host.wait_for("Cert store changed: v2g chain 0, v2g20 chain 1, v2g roots 1, oem roots 0, mo roots 0", timeout=10)

    hd = ca.hash_data(ca.cert_pem, ca.cert_pem)
    assert csms.call("DeleteCertificate", {"certificateHashData": hd})["status"] == "Accepted"
    host.wait_for("Cert store changed: v2g chain 0, v2g20 chain 1, v2g roots 0, oem roots 0, mo roots 0", timeout=10)
