# HUB20-21-005: trust-store changes require an authenticated,
# integrity-protected OCPP connection (Security Profile 2 or 3).

from minicsms import MiniCsms
from testca import SigningCa


def _start(hosts, csms, name, *, ca=None):
    kwargs = {}
    if ca is not None:
        kwargs = {"password": "trust-store-test-password", "ca": str(ca.cert)}
    host = hosts.start(csms.url, name, **kwargs)
    csms.wait_connected()
    host.wait_for("Boot notification accepted", timeout=20)
    return host


def _install(csms, ca):
    return csms.call("InstallCertificate", {
        "certificateType": "V2GRootCertificate",
        "certificate": ca.cert_pem,
    })


def _listed(csms):
    return csms.call("GetInstalledCertificateIds", {
        "certificateType": ["V2GRootCertificate"],
    })


def test_trust_store_changes_require_security_profile_2(hosts, tmp_path):
    ca_dir = tmp_path / "ca"
    ca_dir.mkdir()
    ca = SigningCa(ca_dir, name="tfocpp-trust-store-test-ca")
    server_cert, server_key = ca.server_cert()
    name = "tfocpp-trust-store-security"

    plain = MiniCsms()
    try:
        host = _start(hosts, plain, name)
        assert _install(plain, ca)["status"] == "Rejected"
        assert _listed(plain)["status"] == "NotFound"
        host.stop()
    finally:
        plain.stop()

    secure = MiniCsms(certfile=server_cert, keyfile=server_key)
    try:
        host = _start(hosts, secure, name, ca=ca)
        assert secure.last_auth is not None
        assert _install(secure, ca)["status"] == "Accepted"
        listed = _listed(secure)
        assert listed["status"] == "Accepted"
        hash_data = listed["certificateHashDataChain"][0]["certificateHashData"]
        host.stop()
    finally:
        secure.stop()

    plain = MiniCsms()
    try:
        host = _start(hosts, plain, name)
        assert plain.call("DeleteCertificate", {
            "certificateHashData": hash_data,
        })["status"] == "Failed"
        assert _listed(plain)["status"] == "Accepted"
        host.stop()
    finally:
        plain.stop()

    secure = MiniCsms(certfile=server_cert, keyfile=server_key)
    try:
        _start(hosts, secure, name, ca=ca)
        assert secure.call("DeleteCertificate", {
            "certificateHashData": hash_data,
        })["status"] == "Accepted"
        assert _listed(secure)["status"] == "NotFound"
    finally:
        secure.stop()
