# Local openssl s_server instances for the negative TLS tests.

import socket
import subprocess
import time


def free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def generate_cert(tmp_path, name="localhost"):
    cert = tmp_path / f"{name}.pem"
    key = tmp_path / f"{name}-key.pem"
    subprocess.run([
        "openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
        "-keyout", str(key), "-out", str(cert), "-days", "1",
        "-subj", f"/CN={name}",
        "-addext", f"subjectAltName=DNS:{name}",
    ], check=True, capture_output=True)
    return cert, key


class SServer:
    # extra_args select the failure mode, e.g. ["-tls1_1"] or a cipher list.
    def __init__(self, cert, key, extra_args):
        self.port = free_port()
        self.proc = subprocess.Popen([
            "openssl", "s_server", "-accept", str(self.port),
            "-cert", str(cert), "-key", str(key), "-quiet",
        ] + extra_args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self._wait_listening()

    def _wait_listening(self):
        for _ in range(50):
            try:
                with socket.create_connection(("127.0.0.1", self.port), timeout=0.2):
                    return
            except OSError:
                time.sleep(0.1)
        raise RuntimeError("openssl s_server did not start")

    def stop(self):
        self.proc.terminate()
        self.proc.wait()
