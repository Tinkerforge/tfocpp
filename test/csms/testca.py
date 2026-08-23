# Local test CA for the certificate management tests. The tests act as
# the PKI operator: they sign the CSRs from SignCertificate requests and
# generate OCSP responses for the M06 tests.

import base64
import datetime
import pathlib
import subprocess


def _run(args):
    return subprocess.run(args, check=True, capture_output=True, text=True)


class SigningCa:
    def __init__(self, directory, name="tfocpp-test-ca"):
        self.dir = pathlib.Path(directory)
        self.key = self.dir / "ca-key.pem"
        self.cert = self.dir / "ca.pem"
        self._serial = self.dir / "ca.srl"
        _run(["openssl", "req", "-x509", "-newkey", "ec",
              "-pkeyopt", "ec_paramgen_curve:P-256", "-nodes",
              "-keyout", str(self.key), "-out", str(self.cert),
              "-days", "365", "-subj", f"/CN={name}",
              "-addext", "basicConstraints=critical,CA:TRUE"])

    @property
    def cert_pem(self):
        return self.cert.read_text()

    def sign_csr(self, csr_pem, days=365, ocsp_url=None):
        csr = self.dir / "req.csr"
        leaf = self.dir / "leaf.pem"
        csr.write_text(csr_pem)
        args = ["openssl", "x509", "-req", "-in", str(csr),
                "-CA", str(self.cert), "-CAkey", str(self.key),
                "-CAcreateserial", "-CAserial", str(self._serial),
                "-days", str(days), "-sha256", "-out", str(leaf)]
        if ocsp_url is not None:
            ext = self.dir / "ext.cnf"
            ext.write_text(f"[ext]\nauthorityInfoAccess = OCSP;URI:{ocsp_url}\n")
            args += ["-extfile", str(ext), "-extensions", "ext"]
        _run(args)
        return leaf.read_text()

    def issue_leaf(self, cn):
        # A non-CA end entity certificate, e.g. to test M05 rejection.
        key = self.dir / "leaf-key.pem"
        csr = self.dir / "leaf.csr"
        _run(["openssl", "req", "-newkey", "ec",
              "-pkeyopt", "ec_paramgen_curve:P-256", "-nodes",
              "-keyout", str(key), "-out", str(csr), "-subj", f"/CN={cn}"])
        return self.sign_csr(csr.read_text())

    def server_cert(self, hostname="localhost"):
        # TLS server certificate for a wss test CSMS. Returns (cert, key)
        # file paths.
        key = self.dir / "server-key.pem"
        csr = self.dir / "server.csr"
        cert = self.dir / "server.pem"
        ext = self.dir / "server-ext.cnf"
        _run(["openssl", "req", "-newkey", "ec",
              "-pkeyopt", "ec_paramgen_curve:P-256", "-nodes",
              "-keyout", str(key), "-out", str(csr), "-subj", f"/CN={hostname}"])
        ext.write_text(f"[ext]\nsubjectAltName = DNS:{hostname}\n")
        _run(["openssl", "x509", "-req", "-in", str(csr),
              "-CA", str(self.cert), "-CAkey", str(self.key),
              "-CAcreateserial", "-CAserial", str(self._serial),
              "-days", "365", "-sha256", "-out", str(cert),
              "-extfile", str(ext), "-extensions", "ext"])
        return str(cert), str(key)

    def expired_root(self, name="tfocpp-expired-ca"):
        # A self-signed CA certificate whose validity ended in the past
        # (openssl req -x509 can not set explicit dates before 3.4).
        d = self.dir / "expired"
        d.mkdir(exist_ok=True)
        key = d / "key.pem"
        csr = d / "req.csr"
        cert = d / "cert.pem"
        cnf = d / "ca.cnf"
        (d / "index.txt").write_text("")
        (d / "serial").write_text("01\n")
        cnf.write_text(f"""[ca]
default_ca = ca_default
[ca_default]
database = {d}/index.txt
serial = {d}/serial
new_certs_dir = {d}
default_md = sha256
policy = pol
x509_extensions = ext
email_in_dn = no
[pol]
commonName = supplied
[ext]
basicConstraints = critical,CA:TRUE
""")
        _run(["openssl", "req", "-newkey", "ec",
              "-pkeyopt", "ec_paramgen_curve:P-256", "-nodes",
              "-keyout", str(key), "-out", str(csr), "-subj", f"/CN={name}"])
        _run(["openssl", "ca", "-batch", "-config", str(cnf), "-selfsign",
              "-keyfile", str(key), "-in", str(csr), "-out", str(cert),
              "-startdate", "20200101000000Z", "-enddate", "20210101000000Z",
              "-notext"])
        return cert.read_text()

    def hash_data(self, cert_pem, issuer_pem):
        # OCPP CertificateHashData for cert_pem issued by issuer_pem,
        # hashed per RFC 6960 (issuer key hash over the SPKI bit string).
        import hashlib
        from cryptography import x509
        from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat

        cert = x509.load_pem_x509_certificate(cert_pem.encode())
        issuer = x509.load_pem_x509_certificate(issuer_pem.encode())

        name_hash = hashlib.sha256(cert.issuer.public_bytes()).hexdigest()
        # RFC 6960 issuerKeyHash: over the subjectPublicKey bit string
        # content, which is the uncompressed point for EC keys.
        pubkey = issuer.public_key().public_bytes(Encoding.X962, PublicFormat.UncompressedPoint)
        key_hash = hashlib.sha256(pubkey).hexdigest()

        serial = format(cert.serial_number, "x")
        if len(serial) % 2 != 0:
            serial = "0" + serial
        return {
            "hashAlgorithm": "SHA256",
            "issuerNameHash": name_hash,
            "issuerKeyHash": key_hash,
            "serialNumber": serial,
        }

    def ocsp_response(self, leaf_pem, revoked=False):
        # Base64 encoded DER OCSP response for the leaf, signed by the CA
        # (RFC 6960), as carried in GetCertificateStatusResponse.ocspResult.
        leaf = self.dir / "ocsp-leaf.pem"
        leaf.write_text(leaf_pem)
        serial = _run(["openssl", "x509", "-in", str(leaf), "-noout", "-serial"]).stdout.strip().split("=")[1]

        now = datetime.datetime.now(datetime.timezone.utc)
        expiry = (now + datetime.timedelta(days=365)).strftime("%y%m%d%H%M%SZ")
        revtime = now.strftime("%y%m%d%H%M%SZ")
        index = self.dir / "index.txt"
        if revoked:
            index.write_text(f"R\t{expiry}\t{revtime}\t{serial}\tunknown\t/CN=ocsp-leaf\n")
        else:
            index.write_text(f"V\t{expiry}\t\t{serial}\tunknown\t/CN=ocsp-leaf\n")

        req = self.dir / "ocsp-req.der"
        resp = self.dir / "ocsp-resp.der"
        _run(["openssl", "ocsp", "-issuer", str(self.cert), "-cert", str(leaf),
              "-no_nonce", "-reqout", str(req)])
        _run(["openssl", "ocsp", "-index", str(index), "-CA", str(self.cert),
              "-rsigner", str(self.cert), "-rkey", str(self.key),
              "-reqin", str(req), "-respout", str(resp), "-ndays", "7"])
        return base64.b64encode(resp.read_bytes()).decode()
