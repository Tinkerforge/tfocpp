# M06/M07 OCSP flows against the scripted mini CSMS: evtivity forwards a
# JSON body instead of a DER OCSP request to responders, so the OCSP
# round trip is driven here with locally generated RFC 6960 responses.
# Also covers the A03 renewal trigger, which needs control over the
# certificate validity.

import base64
import datetime
import time

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


@pytest.fixture
def host(csms, hosts, ca):
    h = hosts.start(csms.url, "tfocpp-ocsp-test",
                    password="tfocpp-ocsp-test-password", ca=str(ca.cert))
    csms.wait_connected()
    h.wait_for("Boot notification accepted", timeout=20)
    return h


def install_v2g20_chain(csms, host, ca, days=365, ocsp_url=None):
    return install_v2g_chain(csms, host, ca, True, days=days, ocsp_url=ocsp_url)


def install_v2g_chain(csms, host, ca, iso20, days=365, ocsp_url=None):
    # Trigger one V2G CSR flow and deliver a chain signed by the test CA.
    trigger = "SignV2G20Certificate" if iso20 else "SignV2GCertificate"
    cert_type = "V2G20Certificate" if iso20 else "V2GCertificate"
    assert csms.call("InstallCertificate", {
        "certificateType": "V2GRootCertificate",
        "certificate": ca.cert_pem,
    })["status"] == "Accepted"

    assert csms.call("TriggerMessage", {
        "requestedMessage": trigger,
    })["status"] == "Accepted"

    sign_req, msg_id = csms.expect("SignCertificate")
    assert sign_req["certificateType"] == cert_type
    csms.respond(msg_id, {"status": "Accepted"})

    leaf = ca.sign_csr(sign_req["csr"], days=days, ocsp_url=ocsp_url)
    assert csms.call("CertificateSigned", {
        "certificateChain": leaf,
        "certificateType": cert_type,
        "requestId": sign_req["requestId"],
    })["status"] == "Accepted"

    host.wait_for(f"Installed the signed {cert_type}", timeout=10)
    return leaf


def start_v2g_csr(csms, iso20=False):
    trigger = "SignV2G20Certificate" if iso20 else "SignV2GCertificate"
    cert_type = "V2G20Certificate" if iso20 else "V2GCertificate"
    assert csms.call("TriggerMessage", {"requestedMessage": trigger})["status"] == "Accepted"
    request, message_id = csms.expect("SignCertificate")
    csms.respond(message_id, {"status": "Accepted"})
    assert request["certificateType"] == cert_type
    return request, cert_type


def installed_v2g_chains(csms):
    response = csms.call("GetInstalledCertificateIds", {
        "certificateType": ["V2GCertificateChain"],
    })
    assert response["status"] == "Accepted"
    return response["certificateHashDataChain"]


def test_m03_two_intermediate_order_and_root_inclusion_rejection(csms, host, ca):
    sub1 = ca.intermediate_ca("CPO Sub-CA 1")
    sub2 = sub1.intermediate_ca("CPO Sub-CA 2")
    assert csms.call("InstallCertificate", {
        "certificateType": "V2GRootCertificate",
        "certificate": ca.cert_pem,
    })["status"] == "Accepted"

    request, cert_type = start_v2g_csr(csms)
    leaf = sub2.sign_csr(request["csr"])
    chain = leaf + sub2.cert_pem + sub1.cert_pem
    assert csms.call("CertificateSigned", {
        "certificateChain": chain,
        "certificateType": cert_type,
        "requestId": request["requestId"],
    })["status"] == "Accepted"
    host.wait_for("Installed the signed V2GCertificate", timeout=10)

    installed = installed_v2g_chains(csms)
    assert len(installed) == 1
    entry = installed[0]
    assert entry["certificateHashData"] == sub2.hash_data(leaf, sub2.cert_pem)
    assert entry["childCertificateHashData"] == [
        sub1.hash_data(sub2.cert_pem, sub1.cert_pem),
        ca.hash_data(sub1.cert_pem, ca.cert_pem),
    ]
    root_hash = ca.hash_data(ca.cert_pem, ca.cert_pem)
    assert root_hash not in entry["childCertificateHashData"]

    before = installed
    request, cert_type = start_v2g_csr(csms)
    replacement = sub2.sign_csr(request["csr"])
    rejected = csms.call("CertificateSigned", {
        "certificateChain": replacement + sub2.cert_pem + sub1.cert_pem + ca.cert_pem,
        "certificateType": cert_type,
        "requestId": request["requestId"],
    })
    assert rejected["status"] == "Rejected"
    assert rejected["statusInfo"]["reasonCode"] == "ChainIncludesRoot"
    host.wait_for("CertificateSigned rejected: ChainIncludesRoot", timeout=10)
    assert installed_v2g_chains(csms) == before


def test_certificate_signed_future_validity_boundary(csms, host, ca):
    assert csms.call("InstallCertificate", {
        "certificateType": "V2GRootCertificate",
        "certificate": ca.cert_pem,
    })["status"] == "Accepted"
    epoch = int(time.time())
    host.send(f"time ={epoch}")
    host.wait_for(f"froze system time at {epoch}", timeout=10)
    now = datetime.datetime.fromtimestamp(epoch, datetime.timezone.utc)

    request, cert_type = start_v2g_csr(csms)
    accepted_leaf = ca.sign_csr_at(
        request["csr"], now + datetime.timedelta(seconds=300), now + datetime.timedelta(days=30))
    assert csms.call("CertificateSigned", {
        "certificateChain": accepted_leaf,
        "certificateType": cert_type,
        "requestId": request["requestId"],
    })["status"] == "Accepted"
    host.wait_for("Installed the signed V2GCertificate", timeout=10)
    before = installed_v2g_chains(csms)

    request, cert_type = start_v2g_csr(csms)
    rejected_leaf = ca.sign_csr_at(
        request["csr"], now + datetime.timedelta(seconds=301), now + datetime.timedelta(days=30))
    rejected = csms.call("CertificateSigned", {
        "certificateChain": rejected_leaf,
        "certificateType": cert_type,
        "requestId": request["requestId"],
    })
    assert rejected["status"] == "Rejected"
    assert rejected["statusInfo"]["reasonCode"] == "InvalidChain"
    host.wait_for("CertificateSigned rejected: InvalidChain", timeout=10)
    assert installed_v2g_chains(csms) == before


def test_m06_ocsp_good(csms, host, ca):
    leaf = install_v2g20_chain(csms, host, ca, ocsp_url="http://ocsp.test.example/")

    # M06.FR.06/07: the station requests the status for the new chain.
    status_req, msg_id = csms.expect("GetCertificateStatus")
    data = status_req["ocspRequestData"]
    assert data["hashAlgorithm"] == "SHA256"
    assert data["responderURL"] == "http://ocsp.test.example/"
    assert len(data["issuerNameHash"]) == 64

    csms.respond(msg_id, {"status": "Accepted", "ocspResult": ca.ocsp_response(leaf)})
    host.wait_for("OCSP status good", timeout=10)


@pytest.mark.parametrize("iso20", [False, True], ids=["iso2", "iso20"])
def test_m06_raw_response_retained(csms, host, ca, iso20):
    # Validated Good responses are retained for both SECC protocol groups,
    # byte for byte as delivered, and the aggregated chain status turns good.
    import base64

    leaf = install_v2g_chain(csms, host, ca, iso20, ocsp_url="http://ocsp.test.example/")

    _, msg_id = csms.expect("GetCertificateStatus")
    response_b64 = ca.ocsp_response(leaf)
    csms.respond(msg_id, {"status": "Accepted", "ocspResult": response_b64})
    host.wait_for("OCSP status good", timeout=10)

    host.send("m06dump")
    host.wait_for("m06 chain [0-9]+ group [0-9]+ status good", timeout=10)
    m = host.wait_for("m06 staple chain [0-9]+ idx 0 ([0-9a-f]+)$", timeout=10)
    assert m.group(1) == base64.b64decode(response_b64).hex()


def test_iso2_ocsp_requires_embedded_responder_certificate(csms, host, ca):
    leaf = install_v2g_chain(csms, host, ca, False, ocsp_url="http://ocsp.test.example/")

    _, msg_id = csms.expect("GetCertificateStatus")
    csms.respond(msg_id, {
        "status": "Accepted",
        "ocspResult": ca.ocsp_response(leaf, include_certs=False),
    })

    host.wait_for("OCSP response failed validation, rejected", timeout=10)
    host.send("m06dump")
    host.wait_for("m06 chain [0-9]+ group [0-9]+ status unknown", timeout=10)
    assert host.count("m06 staple chain [0-9]+ idx 0") == 0


@pytest.mark.parametrize(("response_days", "advance_s", "boundary"), [
    (1, 24 * 3600 + 10, "nextUpdate"),
    (30, 7 * 24 * 3600 + 10, "seven-day cap"),
])
def test_m06_expiry_invalidates_status_and_staple(csms, host, ca, response_days, advance_s, boundary):
    # HUB20-431-001/M06.FR.10/V2G20-1021: Good status and retained
    # stapling data expire at nextUpdate or seven days, whichever is
    # earlier. Expiry is fail-closed even while the refresh is pending.
    leaf = install_v2g20_chain(csms, host, ca, ocsp_url="http://ocsp.test.example/")

    _, msg_id = csms.expect("GetCertificateStatus")
    csms.respond(msg_id, {
        "status": "Accepted",
        "ocspResult": ca.ocsp_response(leaf, days=response_days),
    })
    host.wait_for("OCSP status good", timeout=10)

    host.send("m06dump")
    host.wait_for("m06 chain [0-9]+ group [0-9]+ status good", timeout=10)
    host.wait_for("m06 staple chain [0-9]+ idx 0", timeout=10)

    host.send(f"time +{advance_s}")
    host.wait_for("OCSP cache expired for chain certificate", timeout=10)
    host.send("m06dump")
    host.wait_for("m06 chain [0-9]+ group [0-9]+ status unknown", timeout=10)
    assert host.count("m06 staple chain [0-9]+ idx 0") == 1, f"stale staple remained after {boundary}"

    # Expiration also arms an immediate full refresh instead of waiting
    # for the normal one-hour retry interval.
    refresh, _ = csms.expect("GetCertificateStatus", timeout=10)
    assert refresh["ocspRequestData"]["responderURL"] == "http://ocsp.test.example/"


def test_m06_status_unknown_without_response(csms, host, ca):
    # HUB20-532-002 consumers see unknown until a good response exists.
    install_v2g20_chain(csms, host, ca, ocsp_url="http://ocsp.test.example/")

    host.send("m06dump")
    host.wait_for("m06 chain [0-9]+ group [0-9]+ status unknown", timeout=10)


def test_m06_ocsp_incomplete_with_intermediate(csms, host, ca):
    # A good leaf response is insufficient when the intermediate has no AIA,
    # because HUB20-532-002 requires status for every transmitted certificate.
    sub = ca.intermediate_ca()

    assert csms.call("InstallCertificate", {
        "certificateType": "V2GRootCertificate",
        "certificate": ca.cert_pem,
    })["status"] == "Accepted"
    assert csms.call("TriggerMessage", {
        "requestedMessage": "SignV2G20Certificate",
    })["status"] == "Accepted"
    sign_req, msg_id = csms.expect("SignCertificate")
    csms.respond(msg_id, {"status": "Accepted"})

    leaf = sub.sign_csr(sign_req["csr"], ocsp_url="http://ocsp.test.example/")
    assert csms.call("CertificateSigned", {
        "certificateChain": leaf + sub.cert_pem,
        "certificateType": "V2G20Certificate",
        "requestId": sign_req["requestId"],
    })["status"] == "Accepted"
    host.wait_for("Installed the signed V2G20Certificate", timeout=10)

    _, msg_id = csms.expect("GetCertificateStatus")
    csms.respond(msg_id, {"status": "Accepted", "ocspResult": sub.ocsp_response(leaf)})
    host.wait_for("OCSP status good", timeout=10)

    host.send("m06dump")
    host.wait_for("m06 chain [0-9]+ group [0-9]+ status unknown", timeout=10)


def test_m06_ocsp_revoked_deletes_chain(csms, host, ca):
    leaf = install_v2g20_chain(csms, host, ca, ocsp_url="http://ocsp.test.example/")

    _, msg_id = csms.expect("GetCertificateStatus")
    csms.respond(msg_id, {"status": "Accepted", "ocspResult": ca.ocsp_response(leaf, revoked=True)})

    # HUB20-431-003: the SECC chain is deleted immediately.
    host.wait_for("OCSP status revoked, deleting the SECC chain", timeout=10)
    listed = csms.call("GetInstalledCertificateIds", {"certificateType": ["V2GCertificateChain"]})
    assert listed["status"] == "NotFound"


def test_m06_ocsp_invalid_response_rejected(csms, host, ca, tmp_path):
    leaf = install_v2g20_chain(csms, host, ca, ocsp_url="http://ocsp.test.example/")

    # A response signed by an unrelated CA fails RFC 6960 validation
    # (HUB20-431-002). The chain stays installed.
    d = tmp_path / "other-ca"
    d.mkdir()
    other = SigningCa(d, name="tfocpp-unrelated-ca")
    other_leaf = other.sign_csr(_csr_for(other, d))

    _, msg_id = csms.expect("GetCertificateStatus")
    csms.respond(msg_id, {"status": "Accepted", "ocspResult": other.ocsp_response(other_leaf)})

    host.wait_for("OCSP response failed validation, rejected", timeout=10)
    listed = csms.call("GetInstalledCertificateIds", {"certificateType": ["V2GCertificateChain"]})
    assert listed["status"] == "Accepted"


def test_m06_chain_replacement_does_not_retarget_in_flight_response(csms, host, ca):
    old_leaf = install_v2g20_chain(
        csms, host, ca, ocsp_url="http://ocsp.test.example/old")
    _, old_status_id = csms.expect("GetCertificateStatus")
    csms.respond(old_status_id, {
        "status": "Accepted",
        "ocspResult": ca.ocsp_response(old_leaf, days=1),
    })
    host.wait_for("OCSP status good", timeout=10)

    # Prepare the replacement while no OCSP request is in flight. Sending its
    # CertificateSigned request is delayed until the old status refresh is out.
    assert csms.call("TriggerMessage", {
        "requestedMessage": "SignV2G20Certificate",
    })["status"] == "Accepted"
    sign_req, sign_id = csms.expect("SignCertificate")
    csms.respond(sign_id, {"status": "Accepted"})
    new_leaf = ca.sign_csr(
        sign_req["csr"], ocsp_url="http://ocsp.test.example/new")

    host.send(f"time +{24 * 3600 + 10}")
    host.wait_for("OCSP cache expired for chain certificate", timeout=10)
    old_refresh, old_refresh_id = csms.expect("GetCertificateStatus", timeout=10)
    assert old_refresh["ocspRequestData"]["responderURL"].endswith("/old")

    assert csms.call("CertificateSigned", {
        "certificateChain": new_leaf,
        "certificateType": "V2G20Certificate",
        "requestId": sign_req["requestId"],
    })["status"] == "Accepted"
    host.wait_for("Installed the signed V2G20Certificate", min_count=2, timeout=10)

    # This result belongs to the removed chain. It must be discarded instead
    # of being validated against whichever entry reused the old array slot.
    invalid_before = host.count("OCSP response failed validation, rejected")
    csms.respond(old_refresh_id, {
        "status": "Accepted",
        "ocspResult": ca.ocsp_response(old_leaf),
    })
    time.sleep(1)
    assert host.count("OCSP response failed validation, rejected") == invalid_before

    new_status, new_status_id = csms.expect("GetCertificateStatus", timeout=10)
    assert new_status["ocspRequestData"]["responderURL"].endswith("/new")
    new_response = ca.ocsp_response(new_leaf)
    csms.respond(new_status_id, {
        "status": "Accepted",
        "ocspResult": new_response,
    })
    host.wait_for("OCSP status good", min_count=2, timeout=10)

    host.send("m06dump")
    host.wait_for("m06 chain [0-9]+ group [0-9]+ status good", timeout=10)
    staple = host.wait_for("m06 staple chain [0-9]+ idx 0 ([0-9a-f]+)$", timeout=10)
    assert staple.group(1) == base64.b64decode(new_response).hex()


def _csr_for(ca, directory):
    import subprocess
    key = directory / "tmp-key.pem"
    csr = directory / "tmp.csr"
    subprocess.run(["openssl", "req", "-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:P-256",
                    "-nodes", "-keyout", str(key), "-out", str(csr), "-subj", "/CN=tmp"],
                   check=True, capture_output=True)
    return csr.read_text()


def test_a03_renewal_on_expiring_certificate(csms, host, ca):
    # A03.FR.02: a certificate within one month of expiry triggers an
    # autonomous CSR with the root hash of the issuing PKI (A03.FR.23).
    install_v2g20_chain(csms, host, ca, days=20)

    # The expiry check runs shortly after each connect.
    csms.disconnect()
    csms.wait_connected(timeout=30)
    host.wait_for("expires soon, requesting renewal \\(A03\\)", timeout=30)

    sign_req, msg_id = csms.expect("SignCertificate")
    csms.respond(msg_id, {"status": "Accepted"})
    assert sign_req["certificateType"] == "V2G20Certificate"
    assert "hashRootCertificate" in sign_req, "A03 CSR must identify the issuing PKI"
    assert len(sign_req["hashRootCertificate"]["issuerKeyHash"]) == 64


def test_m07_vehicle_chain_status(csms, host):
    # M07 plumbing: the host requests the vehicle chain status (driven by
    # the simulator, the ISO 15118 stack arrives later) and caches the
    # result.
    host.send("m07")
    status_req, msg_id = csms.expect("GetCertificateChainStatus")
    entry = status_req["certificateStatusRequests"][0]
    assert entry["source"] == "OCSP"
    assert entry["certificateHashData"]["serialNumber"] == "1234"

    csms.respond(msg_id, {"certificateStatus": [{
        "certificateHashData": entry["certificateHashData"],
        "source": "OCSP",
        "status": "Good",
        "nextUpdate": "2027-01-01T00:00:00Z",
    }]})
    host.wait_for("Vehicle chain certificate 1234: OCSP status Good", timeout=10)
    host.wait_for("Vehicle chain status response received", timeout=10)


def test_m07_vehicle_chain_order(csms, host):
    # HUB20-432-006: the hash data is sent leaf first (Leaf, Sub2, Sub1)
    # and the cache matches response entries by hash, not position.
    host.send("m07 3")
    status_req, msg_id = csms.expect("GetCertificateChainStatus")
    requests = status_req["certificateStatusRequests"]
    assert [r["certificateHashData"]["serialNumber"] for r in requests] == ["1234", "5678", "9abc"]

    statuses = ["Good", "Good", "Revoked"]
    csms.respond(msg_id, {"certificateStatus": [{
        "certificateHashData": r["certificateHashData"],
        "source": "OCSP",
        "status": s,
        "nextUpdate": "2027-01-01T00:00:00Z",
    } for r, s in zip(reversed(requests), reversed(statuses))]})

    host.wait_for("Vehicle chain certificate 1234: OCSP status Good", timeout=10)
    host.wait_for("Vehicle chain certificate 5678: OCSP status Good", timeout=10)
    host.wait_for("Vehicle chain certificate 9abc: OCSP status Revoked", timeout=10)


def test_m07_missing_entry_stays_uncached(csms, host):
    # HUB20-432-010 support: the response arrival is reported even when
    # entries are missing, the missing certificate stays uncached so the
    # caller can treat it as Unknown.
    host.send("m07 2")
    status_req, msg_id = csms.expect("GetCertificateChainStatus")
    requests = status_req["certificateStatusRequests"]
    assert len(requests) == 2

    csms.respond(msg_id, {"certificateStatus": [{
        "certificateHashData": requests[1]["certificateHashData"],
        "source": "OCSP",
        "status": "Good",
        "nextUpdate": "2027-01-01T00:00:00Z",
    }]})
    host.wait_for("Vehicle chain status response received", timeout=10)
    assert host.count("Vehicle chain certificate 5678: OCSP status Good") == 1
    assert host.count("Vehicle chain certificate 1234") == 0


def test_m07_call_error_reported(csms, host):
    host.send("m07")
    status_req, msg_id = csms.expect("GetCertificateChainStatus")
    csms.respond_error(msg_id, "InternalError")
    host.wait_for("Vehicle chain status request failed", timeout=10)


def sign_charging_station_certificate(csms, host, ca):
    assert csms.call("InstallCertificate", {
        "certificateType": "CSMSRootCertificate",
        "certificate": ca.cert_pem,
    })["status"] == "Accepted"
    assert csms.call("TriggerMessage", {
        "requestedMessage": "SignChargingStationCertificate",
    })["status"] == "Accepted"
    sign_req, msg_id = csms.expect("SignCertificate")
    csms.respond(msg_id, {"status": "Accepted"})
    return sign_req


def test_certificate_signed_invalid_chain_rejected(csms, host, ca, tmp_path):
    # TC_A_14 semantics: a chain that does not validate against the
    # installed roots is rejected and reported as a security event
    # (A02.FR.07).
    import time

    sign_req = sign_charging_station_certificate(csms, host, ca)

    d = tmp_path / "foreign-ca"
    d.mkdir()
    foreign = SigningCa(d, name="tfocpp-foreign-ca")
    assert csms.call("CertificateSigned", {
        "certificateChain": foreign.sign_csr(sign_req["csr"]),
        "certificateType": "ChargingStationCertificate",
        "requestId": sign_req["requestId"],
    })["status"] == "Rejected"
    host.wait_for("CertificateSigned rejected: UntrustedChain", timeout=10)

    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        if any(e["type"] == "InvalidChargingStationCertificate" for e in csms.security_events):
            return
        time.sleep(0.2)
    raise TimeoutError("InvalidChargingStationCertificate security event not received")


def test_charging_station_certificate_not_deletable(csms, host, ca):
    # TC_M_23 semantics: the CSMS client chain can not be deleted.
    sign_req = sign_charging_station_certificate(csms, host, ca)

    leaf = ca.sign_csr(sign_req["csr"])
    assert csms.call("CertificateSigned", {
        "certificateChain": leaf,
        "certificateType": "ChargingStationCertificate",
        "requestId": sign_req["requestId"],
    })["status"] == "Accepted"
    host.wait_for("Installed the signed ChargingStationCertificate", timeout=10)

    assert csms.call("DeleteCertificate", {
        "certificateHashData": ca.hash_data(leaf, ca.cert_pem),
    })["status"] == "Failed"
