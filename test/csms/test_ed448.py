import pathlib
import subprocess

import pytest
from cryptography import x509
from cryptography.hazmat.primitives.asymmetric.ed448 import Ed448PublicKey
from cryptography.x509.oid import NameOID

from host import Host
from minicsms import MiniCsms
from testca import SigningCa


OID_ED448_DER = b"\x30\x05\x06\x03\x2b\x65\x71"


def run(*args):
    return subprocess.run(args, check=True, capture_output=True)


def test_mbedtls_ed448_csr(binary21, tmp_path):
    if pathlib.Path(binary21).name != "ocpp21_linux_mbedtls":
        pytest.skip("requires ocpp21_linux_mbedtls")

    csr_path = tmp_path / "ed448.csr"
    key_path = tmp_path / "ed448.csr.key"
    host = Host(binary21, ["ws://127.0.0.1:9", "ed448-csr-test"], tmp_path)
    try:
        host.send(f"ed448csr {csr_path}")
        host.wait_for(r"Ed448 CSR written", timeout=10)
    finally:
        host.stop()

    csr_pem = csr_path.read_bytes()
    key_pem = key_path.read_bytes()
    csr = x509.load_pem_x509_csr(csr_pem)
    assert csr.is_signature_valid
    assert isinstance(csr.public_key(), Ed448PublicKey)
    assert csr.subject.get_attributes_for_oid(NameOID.COMMON_NAME)[0].value == "DE*TNK*E123456"
    assert csr.subject.get_attributes_for_oid(NameOID.ORGANIZATION_NAME)[0].value == "Tinkerforge GmbH"
    assert csr.subject.get_attributes_for_oid(NameOID.COUNTRY_NAME)[0].value == "DE"
    assert csr.subject.get_attributes_for_oid(NameOID.DOMAIN_COMPONENT)[0].value == "CSO"

    csr_der = run("openssl", "req", "-in", str(csr_path), "-outform", "DER").stdout
    key_der = run("openssl", "pkey", "-in", str(key_path), "-outform", "DER").stdout
    # SPKI and signature AlgorithmIdentifiers in the CSR, one in PKCS#8.
    assert csr_der.count(OID_ED448_DER) == 2
    assert key_der.count(OID_ED448_DER) == 1
    assert b"\x30\x07\x06\x03\x2b\x65\x71\x05\x00" not in csr_der
    assert b"\x30\x07\x06\x03\x2b\x65\x71\x05\x00" not in key_der

    run("openssl", "req", "-in", str(csr_path), "-noout", "-verify")
    run("openssl", "pkey", "-in", str(key_path), "-check", "-noout")
    csr_public = run("openssl", "req", "-in", str(csr_path), "-pubkey", "-noout").stdout
    key_public = run("openssl", "pkey", "-in", str(key_path), "-pubout").stdout
    assert csr_public == key_public
    assert key_pem.startswith(b"-----BEGIN PRIVATE KEY-----\n")

    root_key = tmp_path / "root.key"
    root_cert = tmp_path / "root.pem"
    leaf_cert = tmp_path / "leaf.pem"
    extensions = tmp_path / "leaf.ext"
    extensions.write_text("basicConstraints=critical,CA:FALSE\nkeyUsage=critical,digitalSignature\n")
    run("openssl", "genpkey", "-algorithm", "ED448", "-out", str(root_key))
    run("openssl", "req", "-new", "-x509", "-key", str(root_key), "-out", str(root_cert),
        "-days", "365", "-subj", "/CN=Ed448 Test Root",
        "-addext", "basicConstraints=critical,CA:TRUE",
        "-addext", "keyUsage=critical,keyCertSign,cRLSign")
    run("openssl", "x509", "-req", "-in", str(csr_path), "-CA", str(root_cert),
        "-CAkey", str(root_key), "-CAcreateserial", "-out", str(leaf_cert),
        "-days", "30", "-extfile", str(extensions))

    host = Host(binary21, ["ws://127.0.0.1:9", "ed448-cert-test"], tmp_path)
    try:
        host.send(f"ed448check {key_path.name} {leaf_cert.name} {root_cert.name}")
        match = host.wait_for(r"Ed448 check match (\d) verify (\d+) anchor (\d+) pk_api (\d)", timeout=10)
    finally:
        host.stop()
    assert match.group(1) == "1"
    assert match.group(2) == "0"
    assert match.group(3) == "0"
    assert match.group(4) == "1"


def test_mbedtls_ed448_ocpp_certificate_install(binary21, tmp_path):
    if pathlib.Path(binary21).name != "ocpp21_linux_mbedtls":
        pytest.skip("requires ocpp21_linux_mbedtls")

    transport_dir = tmp_path / "transport-ca"
    transport_dir.mkdir()
    transport_ca = SigningCa(transport_dir)
    server_cert, server_key = transport_ca.server_cert()
    csms = MiniCsms(certfile=server_cert, keyfile=server_key)
    host = Host(binary21, [csms.url, "ed448-ocpp-test", "password",
                           "--ca", str(transport_ca.cert)], tmp_path)
    root_key = tmp_path / "ed448-root.key"
    root_cert = tmp_path / "ed448-root.pem"
    csr_file = tmp_path / "ed448.csr"
    leaf_cert = tmp_path / "ed448-leaf.pem"
    try:
        csms.wait_connected()
        host.wait_for("Boot notification accepted", timeout=20)
        run("openssl", "genpkey", "-algorithm", "ED448", "-out", str(root_key))
        run("openssl", "req", "-new", "-x509", "-key", str(root_key),
            "-out", str(root_cert), "-days", "30", "-subj", "/CN=Ed448 V2G Root",
            "-addext", "basicConstraints=critical,CA:TRUE",
            "-addext", "keyUsage=critical,keyCertSign,cRLSign")

        assert csms.call("InstallCertificate", {
            "certificateType": "V2GRootCertificate",
            "certificate": root_cert.read_text(),
        })["status"] == "Accepted"
        result = csms.call("SetVariables", {"setVariableData": [{
            "component": {"name": "ISO15118Ctrlr"},
            "variable": {"name": "V2G20SECCLeafCryptoSuite"},
            "attributeValue": "ed448",
        }]})["setVariableResult"][0]
        assert result["attributeStatus"] == "Accepted"
        assert csms.call("TriggerMessage", {
            "requestedMessage": "SignV2G20Certificate",
        })["status"] == "Accepted"

        sign_req, sign_id = csms.expect("SignCertificate")
        csms.respond(sign_id, {"status": "Accepted"})
        csr_file.write_text(sign_req["csr"])
        csr = x509.load_pem_x509_csr(sign_req["csr"].encode())
        assert csr.is_signature_valid
        assert isinstance(csr.public_key(), Ed448PublicKey)
        run("openssl", "x509", "-req", "-in", str(csr_file),
            "-CA", str(root_cert), "-CAkey", str(root_key), "-CAcreateserial",
            "-out", str(leaf_cert), "-days", "30")

        response = csms.call("CertificateSigned", {
            "certificateChain": leaf_cert.read_text(),
            "certificateType": "V2G20Certificate",
            "requestId": sign_req["requestId"],
        })
        assert response["status"] == "Accepted"
        host.wait_for("Installed the signed V2G20Certificate", timeout=10)
        installed = csms.call("GetInstalledCertificateIds", {
            "certificateType": ["V2GCertificateChain"],
        })
        assert installed["status"] == "Accepted"
    finally:
        host.stop()
        csms.stop()
