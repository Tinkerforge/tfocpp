# Certificate management: install/list/delete CA certificates and the
# CSR flows for the charging station and V2G certificates. The tests act
# as the PKI: the CSMS uses the manual PnC provider, so CSRs queue up in
# the REST API and the test signs them with a local CA.

import time

import pytest

from conftest import WS, WSS
from testca import SigningCa


def _mkdir(tmp_path, name):
    d = tmp_path / name
    d.mkdir()
    return d


@pytest.fixture
def ca(tmp_path):
    d = tmp_path / "ca"
    d.mkdir()
    return SigningCa(d)


class StationApi:
    def __init__(self, api, name, db_id):
        self.api = api
        self.name = name
        self.db_id = db_id

    def command_response(self, action, **fields):
        since = self.api.last_ocpp_log_id(self.db_id)
        self.api.command(action, self.name, **fields)
        return self.api.wait_for_response(self.db_id, action, since_id=since)

    def install(self, cert_type, pem):
        return self.command_response("InstallCertificate", certificateType=cert_type, certificate=pem)

    def get_installed(self, types=None):
        fields = {"certificateType": types} if types is not None else {}
        return self.command_response("GetInstalledCertificateIds", **fields)

    def delete(self, hash_data):
        return self.command_response("DeleteCertificate", certificateHashData=hash_data)

    def certificate_entries(self):
        payload = self.command_response("GetVariables", getVariableData=[
            {"component": {"name": "SecurityCtrlr"}, "variable": {"name": "CertificateEntries"}},
        ])
        result = payload["getVariableResult"][0]
        assert result["attributeStatus"] == "Accepted"
        return int(result["attributeValue"])


@pytest.fixture
def station(api, stations, hosts, certs):
    name, db_id = stations.create(security_profile=2, password="test-basic-auth-pass-01")
    h = hosts.start(WSS, name, "test-basic-auth-pass-01", ca=certs["ca"])
    h.wait_for("Boot notification accepted", timeout=20)
    return StationApi(api, name, db_id), h


def test_install_list_delete_roots(station, ca, tmp_path):
    s, h = station

    assert s.install("CSMSRootCertificate", ca.cert_pem)["status"] == "Accepted"
    assert s.certificate_entries() == 1

    # M05.FR.17: reinstalling replaces, no duplicate entry.
    assert s.install("CSMSRootCertificate", ca.cert_pem)["status"] == "Accepted"
    assert s.certificate_entries() == 1

    d = tmp_path / "ca2"
    d.mkdir()
    ca2 = SigningCa(d, name="tfocpp-test-ca2")
    assert s.install("V2GRootCertificate", ca2.cert_pem)["status"] == "Accepted"
    assert s.certificate_entries() == 2

    # TC_M_02/M_16/M_101 semantics: manufacturer, MO and OEM roots
    # install and list. Distinct CAs, delete removes all entries sharing
    # hash data.
    ca3 = SigningCa(_mkdir(tmp_path, "ca3"), name="tfocpp-test-ca3")
    ca4 = SigningCa(_mkdir(tmp_path, "ca4"), name="tfocpp-test-ca4")
    ca5 = SigningCa(_mkdir(tmp_path, "ca5"), name="tfocpp-test-ca5")
    assert s.install("ManufacturerRootCertificate", ca3.cert_pem)["status"] == "Accepted"
    assert s.install("OEMRootCertificate", ca4.cert_pem)["status"] == "Accepted"
    assert s.install("MORootCertificate", ca5.cert_pem)["status"] == "Accepted"
    assert s.certificate_entries() == 5

    listed = s.get_installed()
    assert listed["status"] == "Accepted"
    chains = listed["certificateHashDataChain"]
    assert sorted(c["certificateType"] for c in chains) == [
        "CSMSRootCertificate", "MORootCertificate", "ManufacturerRootCertificate",
        "OEMRootCertificate", "V2GRootCertificate"]
    hash_data = chains[0]["certificateHashData"]
    assert hash_data["hashAlgorithm"] == "SHA256"
    assert len(hash_data["issuerNameHash"]) == 64

    filtered = s.get_installed(["V2GRootCertificate"])
    assert len(filtered["certificateHashDataChain"]) == 1
    # TC_M_17 semantics: a multi type filter.
    two = s.get_installed(["CSMSRootCertificate", "ManufacturerRootCertificate"])
    assert len(two["certificateHashDataChain"]) == 2

    for cert_type in ("ManufacturerRootCertificate", "OEMRootCertificate", "MORootCertificate"):
        type_hash = s.get_installed([cert_type])["certificateHashDataChain"][0]["certificateHashData"]
        assert s.delete(type_hash)["status"] == "Accepted"

    # HUB20-413-001: hash fields are matched case-insensitively.
    v2g_hash = filtered["certificateHashDataChain"][0]["certificateHashData"]
    upper = {k: v.upper() if k != "hashAlgorithm" else v for k, v in v2g_hash.items()}
    assert s.delete(upper)["status"] == "Accepted"
    assert s.delete(upper)["status"] == "NotFound"
    # TC_M_19 semantics: NotFound omits certificateHashDataChain.
    not_found = s.get_installed(["V2GRootCertificate"])
    assert not_found["status"] == "NotFound"
    assert "certificateHashDataChain" not in not_found
    assert s.certificate_entries() == 1


def test_install_invalid_certificate_rejected(station, ca):
    s, h = station

    garbage = "-----BEGIN CERTIFICATE-----\nZm9vYmFy\n-----END CERTIFICATE-----\n"
    assert s.install("V2GRootCertificate", garbage)["status"] == "Rejected"

    # M05.FR.07: an end entity certificate is not a CA certificate.
    assert s.install("V2GRootCertificate", ca.issue_leaf("not-a-ca"))["status"] == "Rejected"

    # TC_M_07 semantics: an expired certificate is rejected.
    assert s.install("CSMSRootCertificate", ca.expired_root())["status"] == "Rejected"
    assert s.certificate_entries() == 0


def test_sign_charging_station_certificate(api, stations, hosts, certs, ca):
    # A02 with this test as PKI: trigger, sign the CSR from the
    # SignCertificate request with the test CA, deliver it via
    # CertificateSigned. The station validates the chain against the
    # installed CSMS root, installs it and reconnects. The CSMS's own
    # manual CSR queue is not used, it stores CSRs without the station
    # reference so its operator signing flow can not dispatch.
    name, db_id = stations.create(security_profile=2, password="0123456789abcdef")
    h = hosts.start(WSS, name, password="0123456789abcdef", ca=certs["ca"])
    h.wait_for("Boot notification accepted", timeout=20)
    s = StationApi(api, name, db_id)

    assert s.install("CSMSRootCertificate", ca.cert_pem)["status"] == "Accepted"

    since = api.last_ocpp_log_id(db_id)
    api.command("TriggerMessage", name, requestedMessage="SignChargingStationCertificate")
    h.wait_for("Sending CSR for the ChargingStationCertificate", timeout=10)

    sign_req = api.wait_for_request(db_id, "SignCertificate", since_id=since)
    assert sign_req["certificateType"] == "ChargingStationCertificate"
    assert "BEGIN CERTIFICATE REQUEST" in sign_req["csr"]

    api.command("CertificateSigned", name,
                certificateChain=ca.sign_csr(sign_req["csr"]),
                certificateType="ChargingStationCertificate",
                requestId=sign_req["requestId"])

    h.wait_for("Installed the signed ChargingStationCertificate", timeout=10)
    h.wait_for("Reconnecting with the new charging station certificate", timeout=10)
    h.wait_for("Connected \\(subprotocol ocpp2.1\\)", min_count=2, timeout=30)

    response = api.wait_for_response(db_id, "CertificateSigned", since_id=since)
    assert response["status"] == "Accepted"

    # The certificate survives a restart and is preferred at boot.
    h.stop()
    h2 = hosts.start(WSS, name, password="0123456789abcdef", ca=certs["ca"])
    h2.wait_for("Using the charging station certificate installed via OCPP", timeout=10)
    h2.wait_for("Boot notification accepted", timeout=20)


def test_sign_v2g_certificate(api, station, ca):
    # A02 for the ISO 15118-20 SECC leaf: secp521r1 CSR, validated
    # against the installed V2G root, listed as V2GCertificateChain.
    api.enable_pnc()
    s, h = station

    assert s.install("V2GRootCertificate", ca.cert_pem)["status"] == "Accepted"

    # The CSMS caches the PnC feature flag for up to 60 s and rejects V2G
    # CSRs until it propagates. The station aborts on rejection, so retry
    # the trigger.
    sign_req = None
    deadline = time.monotonic() + 90
    attempt = 0
    while sign_req is None and time.monotonic() < deadline:
        attempt += 1
        since = api.last_ocpp_log_id(s.db_id)
        api.command("TriggerMessage", s.name, requestedMessage="SignV2G20Certificate")
        h.wait_for("Sending CSR for the V2G20Certificate", min_count=attempt, timeout=10)
        try:
            h.wait_for("SignCertificate rejected by the CSMS", min_count=attempt, timeout=5)
            time.sleep(5)
        except TimeoutError:
            sign_req = api.wait_for_request(s.db_id, "SignCertificate", since_id=since)
    assert sign_req is not None, "CSMS kept rejecting the V2G CSR"

    assert sign_req["certificateType"] == "V2G20Certificate"

    # The -20 certificate profile requires a domainComponent ending in
    # "CSO" in the SECC leaf subject (the -2 equivalent is "CPO" per
    # V2G2-875), real V2G PKIs reject CSRs without it.
    from cryptography import x509
    from cryptography.x509.oid import NameOID
    csr = x509.load_pem_x509_csr(sign_req["csr"].encode())
    dc = csr.subject.get_attributes_for_oid(NameOID.DOMAIN_COMPONENT)
    assert len(dc) == 1 and dc[0].value.endswith("CSO")

    api.command("CertificateSigned", s.name,
                certificateChain=ca.sign_csr(sign_req["csr"]),
                certificateType="V2G20Certificate",
                requestId=sign_req["requestId"])

    h.wait_for("Installed the signed V2G20Certificate", timeout=10)

    listed = s.get_installed(["V2GCertificateChain"])
    assert listed["status"] == "Accepted"
    chain = listed["certificateHashDataChain"][0]
    assert chain["certificateType"] == "V2GCertificateChain"

    # M04: the SECC chain is deletable, unlike the CSMS client chain.
    assert s.delete(chain["certificateHashData"])["status"] == "Accepted"


def test_private_key_policy_denies_csr(api, stations, hosts, certs, monkeypatch):
    monkeypatch.setenv("OCPP21_DENY_PRIVATE_KEY_OPERATIONS", "1")
    name, db_id = stations.create(security_profile=2, password="0123456789abcdef")
    h = hosts.start(WSS, name, password="0123456789abcdef", ca=certs["ca"])
    h.wait_for("Boot notification accepted", timeout=20)

    since = api.last_ocpp_log_id(db_id)
    api.command("TriggerMessage", name, requestedMessage="SignV2G20Certificate")
    h.wait_for("Failed to generate a key pair and CSR for the V2G20Certificate", timeout=10)

    assert not list((hosts.workdir / f"{name}.certs").glob("key.*"))
    assert not [entry for entry in api.ocpp_logs(db_id, limit=20)
                if entry["id"] > since and entry["action"] == "SignCertificate"]


def test_certificate_signed_without_csr_rejected(station, ca):
    s, h = station
    response = s.command_response("CertificateSigned", certificateChain=ca.cert_pem)
    assert response["status"] == "Rejected"


def test_csr_retry_and_give_up(api, station):
    # A02.FR.17/18/19: the CSR is resent with doubling backoff and gives
    # up after CertSigningRepeatTimes, resuming only on TriggerMessage.
    s, h = station
    api.set_variable(s.name, "SecurityCtrlr", "CertSigningWaitMinimum", "2")
    api.set_variable(s.name, "SecurityCtrlr", "CertSigningRepeatTimes", "1")

    api.command("TriggerMessage", s.name, requestedMessage="SignChargingStationCertificate")
    h.wait_for("Sending CSR for the ChargingStationCertificate", timeout=10)
    h.wait_for("No CertificateSigned received, resending the CSR", timeout=10)
    h.wait_for("CSR retries exhausted", timeout=15)

    # A new trigger restarts the flow with a fresh CSR.
    api.command("TriggerMessage", s.name, requestedMessage="SignChargingStationCertificate")
    h.wait_for("Sending CSR for the ChargingStationCertificate", min_count=2, timeout=10)
