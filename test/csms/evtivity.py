# REST client for the evtivity CSMS test instance.

import random
import string
import time

import requests


class Evtivity:
    def __init__(self, api_url, admin_email, admin_password):
        self.api = api_url.rstrip("/")
        self.session = requests.Session()
        r = self.session.post(f"{self.api}/v1/auth/login",
                              json={"email": admin_email, "password": admin_password},
                              timeout=10)
        r.raise_for_status()
        self.session.headers["Authorization"] = "Bearer " + r.json()["token"]

    def _unwrap(self, r):
        r.raise_for_status()
        d = r.json()
        return d["data"] if isinstance(d, dict) and "data" in d else d

    def get(self, path, **params):
        return self._unwrap(self.session.get(f"{self.api}{path}", params=params, timeout=10))

    def post(self, path, payload=None):
        return self._unwrap(self.session.post(f"{self.api}{path}", json=payload, timeout=10))

    def first_site_id(self):
        return self.get("/v1/sites")[0]["id"]

    def create_station(self, station_id, security_profile, password=None, protocol="ocpp2.1"):
        payload = {
            "stationId": station_id,
            "siteId": self.first_site_id(),
            "ocppProtocol": protocol,
            "securityProfile": security_profile,
            "model": "tfocpp test host",
        }
        if password is not None:
            payload["password"] = password
        station = self.post("/v1/stations", payload)
        self.post(f"/v1/stations/{station['id']}/approve")
        return station

    def delete_station(self, db_id):
        self.session.delete(f"{self.api}/v1/stations/{db_id}", timeout=10)

    def ensure_free_pricing_group(self):
        # The CSMS payment gate randomly simulates payment failures for demo
        # drivers and then remote stops the session. A free tariff bypasses
        # the gate, making test sessions deterministic. Idempotent.
        for group in self.get("/v1/pricing-groups"):
            if group["name"] == "tfocpp-test-free":
                return group["id"]
        group = self.post("/v1/pricing-groups", {
            "name": "tfocpp-test-free",
            "description": "Free tariff for the tfocpp integration tests",
        })
        self.post(f"/v1/pricing-groups/{group['id']}/tariffs", {
            "name": "free",
            "pricePerKwh": "0",
            "pricePerMinute": "0",
            "pricePerSession": "0",
            "isActive": True,
            "isDefault": True,
        })
        return group["id"]

    def assign_pricing_group(self, station_db_id, group_id):
        self.post(f"/v1/stations/{station_db_id}/pricing-groups", {"pricingGroupId": group_id})

    def command(self, action, station_id, **fields):
        return self.post(f"/v1/ocpp/commands/v21/{action}", {"stationId": station_id, **fields})

    def set_variable(self, station_id, component, variable, value):
        return self.command("SetVariables", station_id, setVariableData=[
            {"component": {"name": component}, "variable": {"name": variable}, "attributeValue": value},
        ])

    def get_variables(self, station_id, pairs):
        return self.command("GetVariables", station_id, getVariableData=[
            {"component": {"name": c}, "variable": {"name": v}} for c, v in pairs
        ])

    def set_credentials(self, db_id, password):
        return self.post(f"/v1/stations/{db_id}/credentials", {"password": password})

    def sessions(self, db_id):
        return self.get(f"/v1/stations/{db_id}/sessions")

    def transaction_events(self, session_id):
        return self.get(f"/v1/sessions/{session_id}/transaction-events")

    def security_events(self, db_id):
        return self.get(f"/v1/stations/{db_id}/security-events")

    def ocpp_logs(self, db_id, limit=100):
        return self.get(f"/v1/stations/{db_id}/ocpp-logs", limit=limit)

    def wait_for_response(self, db_id, action, since_id=0, timeout=15):
        # Waits for an OCPP response (message type 3) sent by the charging
        # station for the given action and returns its payload.
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            for entry in self.ocpp_logs(db_id):
                if entry["id"] <= since_id:
                    continue
                if entry["action"] == action and entry["direction"] == "inbound" and entry["messageType"] == 3:
                    return entry["payload"]
            time.sleep(0.5)
        raise TimeoutError(f"no {action} response from the station within {timeout} s")

    def last_ocpp_log_id(self, db_id):
        logs = self.ocpp_logs(db_id, limit=1)
        return logs[0]["id"] if logs else 0

    def wait_for_ended_session(self, db_id, transaction_id, timeout=20):
        # Waits until the CSMS projected an Ended event for the transaction
        # and returns (session, transaction events sorted by seqNo). The
        # session status is CSMS policy (e.g. payment gate), assert on the
        # events instead.
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            for s in self.sessions(db_id):
                if s["transactionId"] != transaction_id:
                    continue
                events = sorted(self.transaction_events(s["id"]), key=lambda e: e["seqNo"])
                if any(e["eventType"] == "ended" for e in events):
                    return s, events
            time.sleep(0.5)
        raise TimeoutError(f"no ended session for transaction {transaction_id} within {timeout} s")


def random_station_name(prefix):
    suffix = "".join(random.choices(string.ascii_lowercase + string.digits, k=8))
    return f"{prefix}-{suffix}"
