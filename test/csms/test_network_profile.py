# A05 network connection profiles against the mini CSMS:
# SetNetworkProfile validation, security profile downgrade rejection,
# switching via NetworkConfigurationPriority and the boot preference of
# the active profile.

import pytest

from minicsms import MiniCsms
from testca import SigningCa


@pytest.fixture
def ca(tmp_path):
    d = tmp_path / "ca"
    d.mkdir()
    return SigningCa(d)


@pytest.fixture
def csms():
    c = MiniCsms()
    yield c
    c.stop()


def set_profile(csms, slot, url, security_profile, **kwargs):
    connection_data = {
        "ocppVersion": "OCPP21",
        "ocppTransport": "JSON",
        "ocppInterface": "Wired0",
        "messageTimeout": 30,
        "ocppCsmsUrl": url,
        "securityProfile": security_profile,
    }
    connection_data.update(kwargs)
    return csms.call("SetNetworkProfile", {
        "configurationSlot": slot,
        "connectionData": connection_data,
    })


def set_priority(csms, value):
    res = csms.call("SetVariables", {"setVariableData": [{
        "component": {"name": "OCPPCommCtrlr"},
        "variable": {"name": "NetworkConfigurationPriority"},
        "attributeValue": value,
    }]})
    return res["setVariableResult"][0]["attributeStatus"]


def test_setnetworkprofile_validation(csms, hosts, ca):
    h = hosts.start(csms.url, "tfocpp-netprof-val")
    csms.wait_connected()
    h.wait_for("Boot notification accepted", timeout=20)

    assert set_profile(csms, 0, csms.url, 1)["status"] == "Rejected"
    assert set_profile(csms, 99, csms.url, 1)["status"] == "Rejected"
    assert set_profile(csms, 1, csms.url, 1, ocppTransport="SOAP")["status"] == "Rejected"
    assert set_profile(csms, 1, csms.url, 1, ocppVersion="OCPP16")["status"] == "Rejected"
    # TLS profiles need a wss URL.
    assert set_profile(csms, 1, "ws://127.0.0.1:9999", 2)["status"] == "Rejected"
    # No VPN or APN backed interfaces.
    assert set_profile(csms, 1, csms.url, 1, vpn={
        "server": "vpn.example", "user": "u", "password": "p",
        "key": "k", "type": "IKEv2",
    })["status"] == "Rejected"
    assert set_profile(csms, 1, csms.url, 1, basicAuthPassword="short")["status"] == "Rejected"

    # TC_A_20 semantics: a TLS profile without an installed CSMS root is
    # rejected.
    assert set_profile(csms, 1, "wss://127.0.0.1:9999", 2)["status"] == "Rejected"
    assert csms.call("InstallCertificate", {
        "certificateType": "CSMSRootCertificate",
        "certificate": ca.cert_pem,
    })["status"] == "Accepted"
    # TC_A_21 semantics: profile 3 without a charging station certificate
    # is rejected.
    assert set_profile(csms, 1, "wss://127.0.0.1:9999", 3)["status"] == "Rejected"
    assert set_profile(csms, 1, "wss://127.0.0.1:9999", 2)["status"] == "Accepted"

    # NetworkConfigurationPriority accepts only filled slots.
    assert set_priority(csms, "3") == "Rejected"
    assert set_priority(csms, "0") == "Rejected"


def test_downgrade_rejected(hosts, ca):
    # TC_A_22: a profile with a lower security profile is rejected,
    # AllowSecurityProfileDowngrade is not supported (A05.FR.03).
    cert, key = ca.server_cert()
    csms = MiniCsms(certfile=cert, keyfile=key)
    try:
        h = hosts.start(csms.url, "tfocpp-netprof-down",
                        password="netprof-down-password", ca=str(ca.cert))
        csms.wait_connected()
        h.wait_for("Boot notification accepted", timeout=20)

        assert set_profile(csms, 2, "ws://127.0.0.1:9999", 1)["status"] == "Rejected"
    finally:
        csms.stop()


def test_profile_switch_and_boot_preference(csms, hosts):
    b = MiniCsms()
    try:
        h = hosts.start(csms.url, "tfocpp-netprof-switch",
                        password="netprof-switch-password")
        csms.wait_connected()
        h.wait_for("Boot notification accepted", timeout=20)

        assert set_profile(csms, 2, b.url, 1)["status"] == "Accepted"
        assert set_priority(csms, "2") == "Accepted"

        # The station switches to the new endpoint after the response left.
        b.wait_connected(timeout=30)
        h.wait_for("Connecting with network profile slot 2", timeout=15)
        assert b.last_auth is not None, "switched connection lost basic auth"

        # The active profile is preferred at boot: restart pointing at the
        # old endpoint, the station connects to the new one.
        h.stop()
        b.connected.clear()
        h2 = hosts.start(csms.url, "tfocpp-netprof-switch",
                         password="netprof-switch-password")
        h2.wait_for("Using network profile slot 2", timeout=20)
        b.wait_connected(timeout=30)
    finally:
        b.stop()
