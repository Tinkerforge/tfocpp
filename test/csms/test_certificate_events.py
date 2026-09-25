# Hubject catalogue check 47/E5, A02.FR.06 and N07.
from datetime import datetime, timezone

import pytest

from test_iso15118 import ca, csms, host  # noqa: F401 - shared fixtures
from testca import SigningCa


@pytest.mark.parametrize("bundled_root", [False, True])
@pytest.mark.parametrize("unrelated_root", [False, True])
def test_missing_v2g_root_event(csms, host, ca, tmp_path, bundled_root, unrelated_root):
    if unrelated_root:
        directory = tmp_path / "unrelated"
        directory.mkdir()
        other = SigningCa(directory, name="unrelated-root")
        assert csms.call("InstallCertificate", {"certificateType": "V2GRootCertificate", "certificate": other.cert_pem})["status"] == "Accepted"
    reason = "UntrustedChain" if unrelated_root else "NoTrustedRoot"
    assert csms.call("TriggerMessage", {"requestedMessage": "SignV2G20Certificate"})["status"] == "Accepted"
    request, mid = csms.expect("SignCertificate")
    csms.respond(mid, {"status": "Accepted"})
    chain = ca.sign_csr(request["csr"]) + (ca.cert_pem if bundled_root else "")
    payload = {"certificateType": "V2G20Certificate", "requestId": request["requestId"], "certificateChain": chain}
    event_ids = []
    for _ in range(2):
        rejected = csms.call("CertificateSigned", payload)
        assert rejected["status"] == "Rejected"
        assert rejected["statusInfo"]["reasonCode"] == reason
        event, mid = csms.expect("NotifyEvent", timeout=3)
        csms.respond(mid, {})
        assert event["seqNo"] == 0
        assert event.get("tbc", False) is False
        assert len(event["eventData"]) == 1
        data = event["eventData"][0]
        assert data["component"] == {"name": "SecurityCtrlr"}
        assert data["variable"] == {"name": "CertificateEntries"}
        assert data["actualValue"] == str(int(unrelated_root))
        assert data["trigger"] == "Alerting"
        assert data["eventNotificationType"] == "HardWiredNotification"
        assert data["severity"] == 3
        assert data["techCode"] == reason
        assert data["techInfo"] == "V2GCertificateChain installation failed because the corresponding V2G root was not found."
        assert "variableMonitoringId" not in data
        assert "cleared" not in data
        assert isinstance(data["eventId"], int) and data["eventId"] >= 0
        event_ids.append(data["eventId"])
        for stamp in (event["generatedAt"], data["timestamp"]):
            assert abs((datetime.now(timezone.utc) - datetime.fromisoformat(stamp.replace("Z", "+00:00"))).total_seconds()) < 30
    assert event_ids[0] != event_ids[1]
    assert csms.call("GetInstalledCertificateIds", {"certificateType": ["V2GCertificateChain"]})["status"] == "NotFound"
    assert not csms.security_events

    # The rejected request is recoverable after independent root provisioning.
    assert csms.call("InstallCertificate", {"certificateType": "V2GRootCertificate", "certificate": ca.cert_pem})["status"] == "Accepted"
    assert csms.call("CertificateSigned", payload)["status"] == "Accepted"
    assert csms.call("GetInstalledCertificateIds", {"certificateType": ["V2GCertificateChain"]})["status"] == "Accepted"
    with pytest.raises(TimeoutError):
        csms.expect("NotifyEvent", timeout=0.5)
    assert not csms.security_events


@pytest.mark.parametrize("certificate_type,trigger", [
    ("V2GCertificate", "SignV2GCertificate"),
    ("V2G20Certificate", "SignV2G20Certificate"),
])
def test_unrelated_rejections_do_not_report_missing_root(csms, host, ca, certificate_type, trigger):
    assert csms.call("TriggerMessage", {"requestedMessage": trigger})["status"] == "Accepted"
    request, mid = csms.expect("SignCertificate")
    csms.respond(mid, {"status": "Accepted"})
    payload = {"certificateType": certificate_type, "requestId": request["requestId"] + 1,
               "certificateChain": ca.sign_csr(request["csr"])}
    result = csms.call("CertificateSigned", payload)
    assert result["statusInfo"]["reasonCode"] == "UnknownRequestId"
    with pytest.raises(TimeoutError):
        csms.expect("NotifyEvent", timeout=0.5)
    if certificate_type == "V2GCertificate":
        payload["requestId"] = request["requestId"]
        assert csms.call("CertificateSigned", payload)["statusInfo"]["reasonCode"] == "NoTrustedRoot"
        with pytest.raises(TimeoutError):
            csms.expect("NotifyEvent", timeout=0.5)
    assert not csms.security_events
