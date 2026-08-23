# Security profiles 2/3, TLS failure security events, BasicAuthPassword update.

import json
import subprocess
import time

import pytest

from conftest import WS, WSS, DOCKER_OCPP, DOCKER_PG
import tlsserver

PASSWORD = "test-security-profile-2-pass"


def start_sec2(api, stations, hosts, certs):
    name, db_id = stations.create(security_profile=2, password=PASSWORD)
    h = hosts.start(WSS, name, PASSWORD, ca=certs["ca"])
    h.wait_for("Boot notification accepted")
    return name, db_id, h


def test_profile2_boot_and_session(api, stations, hosts, certs, rfid):
    name, db_id, h = start_sec2(api, stations, hosts, certs)

    h.send(f"tag {rfid}")
    h.wait_for(f"Authorization of {rfid} accepted")
    h.send("plug")
    transaction_id = h.wait_for("Starting transaction ([0-9a-f-]+)").group(1)
    h.send(f"tag {rfid}")
    h.wait_for("Stopping transaction")
    h.send("unplug")

    api.wait_for_ended_session(db_id, transaction_id)


@pytest.mark.docker
def test_profile3_mtls(api, stations, hosts, certs):
    name, db_id = stations.create(security_profile=3)

    # Register the client certificate for the station. The REST install
    # endpoint dispatches OCPP InstallCertificate and needs a connected
    # station, so initial onboarding writes the row directly.
    serial = subprocess.run(["openssl", "x509", "-in", certs["cert"], "-noout", "-serial"],
                            check=True, capture_output=True, text=True).stdout.strip().split("=")[1]
    pem = open(certs["cert"]).read()
    sql = ("INSERT INTO station_certificates (station_id, certificate_type, certificate, serial_number, status, source) "
           f"VALUES ('{db_id}', 'ChargingStationCertificate', '{pem}', '{serial}', 'active', 'manual');")
    subprocess.run(["docker", "exec", "-i", DOCKER_PG, "psql", "-U", "evtivity"],
                   input=sql, check=True, capture_output=True, text=True)

    h = hosts.start(WSS, name, ca=certs["ca"], cert=certs["cert"], key=certs["key"])
    h.wait_for("Boot notification accepted")


def test_invalid_csms_certificate_reported_once(api, stations, hosts, certs, tmp_path):
    # A CA that did not sign the CSMS certificate must be classified as
    # InvalidCsmsCertificate and reported once per failure streak.
    bogus_ca, _ = tlsserver.generate_cert(tmp_path, "bogus-ca")
    name, db_id = stations.create(security_profile=2, password=PASSWORD)
    h = hosts.start(WSS, name, PASSWORD, ca=str(bogus_ca))

    h.wait_for("TLS connection failed: InvalidCsmsCertificate")
    # Wait through at least one reconnect cycle (10 s interval).
    time.sleep(12)
    assert h.count("tls hs:") >= 2, "expected at least two handshake attempts"
    assert h.count("TLS connection failed: InvalidCsmsCertificate") == 1


@pytest.mark.docker
def test_hostname_mismatch(api, stations, hosts, certs):
    # The container IP is not in the certificate SAN.
    ip = subprocess.run(["docker", "inspect", DOCKER_OCPP, "--format",
                         "{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}"],
                        check=True, capture_output=True, text=True).stdout.strip()
    port = WSS.rsplit(":", 1)[1].split("/")[0]
    name, db_id = stations.create(security_profile=2, password=PASSWORD)
    h = hosts.start(f"wss://{ip}:{port}", name, PASSWORD, ca=certs["ca"])
    h.wait_for("TLS connection failed: InvalidCsmsCertificate")


def test_invalid_tls_version(hosts, tmp_path):
    cert, key = tlsserver.generate_cert(tmp_path)
    server = tlsserver.SServer(cert, key, ["-tls1_1", "-cipher", "ALL:@SECLEVEL=0"])
    try:
        h = hosts.start(f"wss://localhost:{server.port}", "tls-test-host", "irrelevant-password-16", ca=str(cert))
        h.wait_for("TLS connection failed: InvalidTLSVersion")
    finally:
        server.stop()


def test_invalid_tls_cipher_suite(hosts, tmp_path):
    cert, key = tlsserver.generate_cert(tmp_path)
    server = tlsserver.SServer(cert, key, ["-tls1_2", "-cipher", "AES128-SHA256:@SECLEVEL=0"])
    try:
        h = hosts.start(f"wss://localhost:{server.port}", "tls-test-host", "irrelevant-password-16", ca=str(cert))
        h.wait_for("TLS connection failed: InvalidTLSCipherSuite")
    finally:
        server.stop()


def test_security_event_delivery(api, stations, hosts, certs):
    name, db_id, h = start_sec2(api, stations, hosts, certs)

    h.send("secevent SettingSystemTime")
    h.wait_for("Received result for SecurityEventNotification")

    events = api.security_events(db_id)
    assert any(e["type"] == "SettingSystemTime" for e in events), events


def test_password_update(api, stations, hosts, certs):
    name, db_id, h = start_sec2(api, stations, hosts, certs)

    new_password = "updated-password-for-a01-test-1"
    api.set_credentials(db_id, new_password)
    h.wait_for("BasicAuthPassword updated, reconnecting with the new password")
    h.wait_for("Connected \\(subprotocol ocpp2.1\\)", min_count=2, timeout=30)

    # The connection authenticates with the new password, traffic continues.
    since = api.last_ocpp_log_id(db_id)
    api.get_variables(name, [("SecurityCtrlr", "SecurityProfile")])
    api.wait_for_response(db_id, "GetVariables", since)

    # Persisted to <name>.sec21 in the working directory.
    persisted = json.load(open(hosts.workdir / f"{name}.sec21"))
    assert persisted["basic_auth_password"] == new_password

    # A restart without a configured password uses the persisted one.
    h.stop()
    h2 = hosts.start(WSS, name, ca=certs["ca"])
    h2.wait_for("Connected \\(subprotocol ocpp2.1\\)")


def test_password_too_short_rejected(api, stations, hosts, certs):
    name, db_id, h = start_sec2(api, stations, hosts, certs)

    since = api.last_ocpp_log_id(db_id)
    api.set_variable(name, "SecurityCtrlr", "BasicAuthPassword", "too-short")
    result = api.wait_for_response(db_id, "SetVariables", since)
    assert result["setVariableResult"][0]["attributeStatus"] == "Rejected"
    assert h.count("BasicAuthPassword updated") == 0


def test_security_ctrlr_variables(api, stations, hosts, certs):
    name, db_id, h = start_sec2(api, stations, hosts, certs)

    since = api.last_ocpp_log_id(db_id)
    api.set_variable(name, "SecurityCtrlr", "OrganizationName", "Test Organization")
    assert api.wait_for_response(db_id, "SetVariables", since)["setVariableResult"][0]["attributeStatus"] == "Accepted"

    since = api.last_ocpp_log_id(db_id)
    api.get_variables(name, [
        ("SecurityCtrlr", "SecurityProfile"),
        ("SecurityCtrlr", "Identity"),
        ("SecurityCtrlr", "OrganizationName"),
        ("SecurityCtrlr", "CertificateEntries"),
        ("SecurityCtrlr", "BasicAuthPassword"),
    ])
    result = api.wait_for_response(db_id, "GetVariables", since)["getVariableResult"]
    by_variable = {r["variable"]["name"]: r for r in result}
    assert by_variable["SecurityProfile"]["attributeValue"] == "2"
    assert by_variable["Identity"]["attributeValue"] == name
    assert by_variable["OrganizationName"]["attributeValue"] == "Test Organization"
    assert by_variable["CertificateEntries"]["attributeValue"] == "0"
    # WriteOnly, reads are rejected.
    assert by_variable["BasicAuthPassword"]["attributeStatus"] == "Rejected"
    assert "attributeValue" not in by_variable["BasicAuthPassword"] or not by_variable["BasicAuthPassword"].get("attributeValue")
