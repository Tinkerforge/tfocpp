# Fixtures for the CSMS integration tests, see README.md.

import os
import itertools
import pathlib
import shutil
import subprocess

import pytest
import requests

from evtivity import Evtivity, random_station_name
from host import Host

TEST_DIR = pathlib.Path(__file__).parent
REPO_ROOT = TEST_DIR.parent.parent


def load_env():
    env_file = TEST_DIR / ".env"
    if not env_file.exists():
        return
    for line in env_file.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, _, value = line.partition("=")
        os.environ.setdefault(key.strip(), value.strip())


load_env()

API = os.environ.get("EVTIVITY_API", "http://localhost:7102")
WS = os.environ.get("EVTIVITY_WS", "ws://localhost:7103")
WSS = os.environ.get("EVTIVITY_WSS", "wss://localhost:8443")
ADMIN_EMAIL = os.environ.get("EVTIVITY_ADMIN_EMAIL", "admin@evtivity.local")
ADMIN_PASSWORD = os.environ.get("EVTIVITY_ADMIN_PASSWORD", "admin123")
DOCKER_OCPP = os.environ.get("EVTIVITY_DOCKER_OCPP", "evtivity-ocpp-1")
DOCKER_PG = os.environ.get("EVTIVITY_DOCKER_PG", "evtivity-postgres-1")
# Each test uses its own token: the CSMS tracks payment state per driver and
# may stop sessions of a driver whose previous session hit the payment gate.
RFID_TOKENS = os.environ.get(
    "RFID_TOKENS",
    ",".join(f"RFID-{i:06d}" for i in range(1, 11)),
).split(",")
TFOCPP_DIR = pathlib.Path(os.environ.get("TFOCPP_DIR", str(REPO_ROOT)))

BINARY21 = TFOCPP_DIR / "ocpp21_linux"
BINARY16 = TFOCPP_DIR / "ocpp16_linux"


def pytest_configure(config):
    config.addinivalue_line("markers", "docker: needs local docker access to the CSMS containers")


def docker_available():
    if shutil.which("docker") is None:
        return False
    return subprocess.run(["docker", "inspect", DOCKER_OCPP],
                          capture_output=True).returncode == 0


def pytest_collection_modifyitems(config, items):
    if docker_available():
        return
    skip = pytest.mark.skip(reason="local docker with the CSMS containers not available")
    for item in items:
        if "docker" in item.keywords:
            item.add_marker(skip)


@pytest.fixture(scope="session")
def api():
    try:
        return Evtivity(API, ADMIN_EMAIL, ADMIN_PASSWORD)
    except requests.RequestException as e:
        pytest.exit(f"CSMS not reachable at {API}: {e}", returncode=3)


_rfid_counter = itertools.count()


@pytest.fixture
def rfid():
    return RFID_TOKENS[next(_rfid_counter) % len(RFID_TOKENS)]


@pytest.fixture(scope="session")
def binary21():
    if not BINARY21.exists():
        pytest.exit(f"{BINARY21} not built, run: make ocpp21_linux", returncode=3)
    return str(BINARY21)


@pytest.fixture(scope="session")
def binary16():
    if not BINARY16.exists():
        pytest.skip(f"{BINARY16} not built, run: make ocpp16_linux")
    return str(BINARY16)


@pytest.fixture(scope="session")
def certs(tmp_path_factory):
    # CA, client cert and key of the CSMS wss endpoint: from the env or
    # extracted from the ocpp container (the certs in the evtivity checkout
    # can be older than the ones baked into the image).
    ca = os.environ.get("EVTIVITY_CA_FILE")
    cert = os.environ.get("EVTIVITY_CLIENT_CERT")
    key = os.environ.get("EVTIVITY_CLIENT_KEY")
    if ca and cert and key:
        return {"ca": ca, "cert": cert, "key": key}

    if not docker_available():
        pytest.skip("no TLS certs in the env and no local docker to extract them")

    d = tmp_path_factory.mktemp("csms-certs")
    files = {"ca": "ca.pem", "cert": "client.pem", "key": "client-key.pem"}
    out = {}
    for k, name in files.items():
        target = d / name
        subprocess.run(["docker", "cp", f"{DOCKER_OCPP}:/app/packages/css/test-certs/{name}", str(target)],
                       check=True, capture_output=True)
        out[k] = str(target)
    return out


class StationFactory:
    def __init__(self, api, pricing_group_id):
        self.api = api
        self.pricing_group_id = pricing_group_id
        self.created = []

    def create(self, security_profile, password=None, prefix="tfocpp-test"):
        name = random_station_name(prefix)
        station = self.api.create_station(name, security_profile, password)
        self.api.assign_pricing_group(station["id"], self.pricing_group_id)
        self.created.append(station["id"])
        return name, station["id"]

    def cleanup(self):
        for db_id in self.created:
            self.api.delete_station(db_id)


@pytest.fixture(scope="session")
def free_pricing_group(api):
    return api.ensure_free_pricing_group()


@pytest.fixture(scope="session", autouse=True)
def purge_blocked_test_stations():
    # Station DELETE is a soft delete in the CSMS, blocked test stations accumulate, delete them
    yield
    if not docker_available():
        return
    subprocess.run(["docker", "exec", DOCKER_PG, "psql", "-U", "evtivity", "-c",
                    "DELETE FROM charging_stations WHERE onboarding_status='blocked'"
                    " AND station_id LIKE 'tfocpp-test-%'"],
                   capture_output=True)


@pytest.fixture
def stations(api, free_pricing_group):
    factory = StationFactory(api, free_pricing_group)
    yield factory
    factory.cleanup()


class HostFactory:
    def __init__(self, binary, workdir):
        self.binary = binary
        self.workdir = workdir
        self.hosts = []

    def start(self, url, name, password=None, ca=None, cert=None, key=None, extra=None):
        args = [url, name]
        if password is not None:
            args.append(password)
        if ca is not None:
            args += ["--ca", ca]
        if cert is not None:
            args += ["--cert", cert]
        if key is not None:
            args += ["--key", key]
        if extra is not None:
            args += extra
        h = Host(self.binary, args, cwd=self.workdir)
        self.hosts.append(h)
        return h

    def stop_all(self):
        for h in self.hosts:
            h.stop()


@pytest.fixture
def hosts(binary21, tmp_path):
    # Each test gets a fresh working directory so persisted state
    # (<name>.sec21) is isolated.
    factory = HostFactory(binary21, tmp_path)
    yield factory
    factory.stop_all()
