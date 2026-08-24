# TransactionEvent based charging sessions.

import subprocess
import time

import pytest

from conftest import WS, DOCKER_OCPP, RFID_TOKENS
from minicsms import MiniCsms

STARTING = "Starting transaction ([0-9a-f-]+)"


@pytest.fixture
def csms():
    c = MiniCsms()
    yield c
    c.stop()


def start_station(api, stations, hosts):
    name, db_id = stations.create(security_profile=1, password="test-basic-auth-pass-01")
    h = hosts.start(WS, name, "test-basic-auth-pass-01")
    h.wait_for("Boot notification accepted")
    return name, db_id, h


def test_local_start_stop(api, stations, hosts, rfid):
    name, db_id, h = start_station(api, stations, hosts)

    h.send(f"tag {rfid}")
    h.wait_for(f"Authorization of {rfid} accepted")
    h.send("plug")
    transaction_id = h.wait_for(STARTING).group(1)

    h.send(f"tag {rfid}")
    h.wait_for("Stopping transaction")
    h.send("unplug")

    session, events = api.wait_for_ended_session(db_id, transaction_id)
    assert events[0]["eventType"] == "started"
    assert events[-1]["eventType"] == "ended"
    assert all(not e["offline"] for e in events)


def test_remote_start_stop(api, stations, hosts, rfid):
    name, db_id, h = start_station(api, stations, hosts)

    api.command("RequestStartTransaction", name, remoteStartId=1,
                idToken={"idToken": rfid, "type": "ISO14443"})
    h.wait_for("Remote start accepted")
    h.send("plug")
    transaction_id = h.wait_for(STARTING).group(1)

    api.command("RequestStopTransaction", name, transactionId=transaction_id)
    h.wait_for("Remote stop accepted")
    h.wait_for("Stopping transaction")
    h.send("unplug")

    session, events = api.wait_for_ended_session(db_id, transaction_id)
    assert events[0]["triggerReason"] == "RemoteStart"


def test_periodic_meter_values(api, stations, hosts, rfid):
    name, db_id, h = start_station(api, stations, hosts)

    since = api.last_ocpp_log_id(db_id)
    api.set_variable(name, "SampledDataCtrlr", "TxUpdatedInterval", "2")
    api.wait_for_response(db_id, "SetVariables", since)

    h.send(f"tag {rfid}")
    h.wait_for(f"Authorization of {rfid} accepted")
    h.send("plug")
    transaction_id = h.wait_for(STARTING).group(1)

    time.sleep(7)
    h.send(f"tag {rfid}")
    h.wait_for("Stopping transaction")
    h.send("unplug")

    session, events = api.wait_for_ended_session(db_id, transaction_id)
    updated = [e for e in events if e["eventType"] == "updated"]
    assert len(updated) >= 2, f"expected periodic Updated events, got {events}"


def test_ev_connection_timeout(api, stations, hosts, rfid):
    name, db_id, h = start_station(api, stations, hosts)

    since = api.last_ocpp_log_id(db_id)
    api.set_variable(name, "TxCtrlr", "EVConnectionTimeOut", "2")
    api.wait_for_response(db_id, "SetVariables", since)

    h.send(f"tag {rfid}")
    h.wait_for(f"Authorization of {rfid} accepted")
    # No plug follows, the authorization must be canceled.
    h.wait_for("EV connection timeout", timeout=15)


def test_suspended_charging_states(api, stations, hosts, rfid):
    name, db_id, h = start_station(api, stations, hosts)

    h.send(f"tag {rfid}")
    h.wait_for(f"Authorization of {rfid} accepted")
    h.send("plug")
    transaction_id = h.wait_for(STARTING).group(1)

    time.sleep(1)
    h.send("suspend")
    time.sleep(1)
    h.send("resume")
    time.sleep(1)

    h.send(f"tag {rfid}")
    h.wait_for("Stopping transaction")
    h.send("unplug")

    api.wait_for_ended_session(db_id, transaction_id)

    frames = [entry["payload"] for entry in api.ocpp_logs(db_id, limit=100)
              if entry["action"] == "TransactionEvent" and entry["direction"] == "inbound"
              and entry["messageType"] == 2
              and entry["payload"].get("transactionInfo", {}).get("transactionId") == transaction_id]
    frames.sort(key=lambda f: f["seqNo"])
    states = [f["transactionInfo"].get("chargingState") for f in frames]

    # Started leaves before the power path is closed, then the EV charges,
    # suspends on its own and resumes.
    assert states[0] == "SuspendedEVSE", states
    assert "SuspendedEV" in states, states
    i = states.index("SuspendedEV")
    assert "Charging" in states[:i], states
    assert "Charging" in states[i + 1:], states


def test_tag_feedback(api, stations, hosts, rfid):
    name, db_id, h = start_station(api, stations, hosts)

    # Unknown token: the CSMS rejects, the platform gets the feedback.
    h.send("tag NOT-A-KNOWN-TOKEN")
    h.wait_for("EVSE 1 tag NOT-A-KNOWN-TOKEN rejected")

    h.send(f"tag {rfid}")
    h.wait_for(f"EVSE 1 tag {rfid} accepted")
    h.send("plug")
    transaction_id = h.wait_for(STARTING).group(1)

    # The same tag stops the transaction and is acknowledged again.
    h.send(f"tag {rfid}")
    h.wait_for(f"EVSE 1 tag {rfid} accepted", min_count=2)
    h.wait_for("Stopping transaction")
    h.send("unplug")

    api.wait_for_ended_session(db_id, transaction_id)


def test_plug_first_tag_timeout(api, stations, hosts, rfid):
    name, db_id, h = start_station(api, stations, hosts)

    since = api.last_ocpp_log_id(db_id)
    api.set_variable(name, "TxCtrlr", "EVConnectionTimeOut", "2")
    api.wait_for_response(db_id, "SetVariables", since)

    # A plugged EV without a tag times out, 1.6 ConnectionTimeOut behavior.
    h.send("plug")
    h.wait_for("EVSE 1 expects a tag")
    h.wait_for("EVSE 1 tag timed out", timeout=15)

    # Tags are ignored until a replug.
    h.send(f"tag {rfid}")
    h.wait_for("Ignored until replug")

    h.send("unplug")
    h.send("plug")
    h.wait_for("EVSE 1 expects a tag", min_count=2)
    h.send(f"tag {rfid}")
    h.wait_for(f"EVSE 1 tag {rfid} accepted")
    transaction_id = h.wait_for(STARTING).group(1)

    h.send(f"tag {rfid}")
    h.wait_for("Stopping transaction")
    h.send("unplug")
    api.wait_for_ended_session(db_id, transaction_id)


def test_stop_with_different_tag(api, stations, hosts, rfid):
    other = RFID_TOKENS[(RFID_TOKENS.index(rfid) + 1) % len(RFID_TOKENS)]
    name, db_id, h = start_station(api, stations, hosts)

    h.send(f"tag {rfid}")
    h.wait_for(f"Authorization of {rfid} accepted")
    h.send("plug")
    transaction_id = h.wait_for(STARTING).group(1)

    # An unknown token is authorized against the CSMS and rejected, the
    # transaction continues.
    h.send("tag NOT-A-KNOWN-TOKEN")
    h.wait_for("Authorizing it to stop the transaction")
    h.wait_for("EVSE 1 tag NOT-A-KNOWN-TOKEN rejected")
    assert h.count("Stopping transaction") == 0

    # A different known token stops the transaction after authorization.
    h.send(f"tag {other}")
    h.wait_for(f"Authorization of {other} accepted, stopping the transaction")
    h.wait_for("Stopping transaction")
    h.send("unplug")

    api.wait_for_ended_session(db_id, transaction_id)

    # The Ended event reports the token that stopped the transaction.
    frames = [entry["payload"] for entry in api.ocpp_logs(db_id, limit=100)
              if entry["action"] == "TransactionEvent" and entry["direction"] == "inbound"
              and entry["messageType"] == 2 and entry["payload"].get("eventType") == "Ended"
              and entry["payload"].get("transactionInfo", {}).get("transactionId") == transaction_id]
    assert frames and frames[0].get("idToken", {}).get("idToken") == other, frames


def test_local_stop_keeps_cable_locked(api, stations, hosts, rfid):
    name, db_id, h = start_station(api, stations, hosts)

    h.send(f"tag {rfid}")
    h.wait_for(f"Authorization of {rfid} accepted")
    h.send("plug")
    transaction_id = h.wait_for(STARTING).group(1)
    h.wait_for("EVSE 1 cable locked")

    h.send("stop emergency")
    h.wait_for("Stopping transaction")
    time.sleep(0.5)
    assert h.count("EVSE 1 cable unlocked") == 0

    # Only the token of the stopped transaction unlocks the cable.
    h.send("tag NOT-A-KNOWN-TOKEN")
    h.wait_for("does not match the token of the stopped transaction")
    assert h.count("EVSE 1 cable unlocked") == 0

    h.send(f"tag {rfid}")
    h.wait_for("EVSE 1 cable unlocked")
    h.send("unplug")

    api.wait_for_ended_session(db_id, transaction_id)

    frames = [entry["payload"] for entry in api.ocpp_logs(db_id, limit=100)
              if entry["action"] == "TransactionEvent" and entry["direction"] == "inbound"
              and entry["messageType"] == 2 and entry["payload"].get("eventType") == "Ended"
              and entry["payload"].get("transactionInfo", {}).get("transactionId") == transaction_id]
    assert frames and frames[0]["transactionInfo"].get("stoppedReason") == "EmergencyStop", frames


@pytest.mark.docker
def test_offline_transaction_continuation(api, stations, hosts, rfid):
    name, db_id, h = start_station(api, stations, hosts)

    h.send(f"tag {rfid}")
    h.wait_for(f"Authorization of {rfid} accepted")
    h.send("plug")
    transaction_id = h.wait_for(STARTING).group(1)

    subprocess.run(["docker", "stop", DOCKER_OCPP], check=True, capture_output=True)
    try:
        h.wait_for("Disconnected", timeout=60)
        # Stop the transaction while offline. The Ended event must be queued.
        h.send(f"tag {rfid}")
        h.wait_for("Stopping transaction")
        h.send("unplug")
    finally:
        subprocess.run(["docker", "start", DOCKER_OCPP], check=True, capture_output=True)

    h.wait_for("Connected \\(subprotocol ocpp2.1\\)", min_count=2, timeout=90)

    session, events = api.wait_for_ended_session(db_id, transaction_id, timeout=30)
    assert any(e["eventType"] == "ended" for e in events)

    # The transaction-events endpoint does not expose the offline flag,
    # check the raw frame in the OCPP log instead.
    frames = [entry["payload"] for entry in api.ocpp_logs(db_id, limit=100)
              if entry["action"] == "TransactionEvent" and entry["direction"] == "inbound"
              and entry["messageType"] == 2 and entry["payload"].get("eventType") == "Ended"]
    assert frames and frames[0].get("offline") is True, f"expected offline Ended frame, got {frames}"


def test_transaction_event_call_error_retries(csms, hosts, rfid):
    h = hosts.start(csms.url, "tfocpp-txn-retry")
    csms.wait_connected()
    h.wait_for("Boot notification accepted", timeout=20)

    res = csms.call("SetVariables", {"setVariableData": [{
        "component": {"name": "OCPPCommCtrlr"},
        "variable": {"name": "MessageAttemptInterval"},
        "attributeValue": "1",
    }]})
    assert res["setVariableResult"][0]["attributeStatus"] == "Accepted"

    h.send(f"tag {rfid}")
    payload, msg_id = csms.expect("Authorize")
    csms.respond(msg_id, {"idTokenInfo": {"status": "Accepted"}})
    h.wait_for(f"Authorization of {rfid} accepted")
    h.send("plug")
    h.wait_for(STARTING)

    # The Started event fails with a CallError MessageAttempts (3) times,
    # retried with a growing backoff, then it is dropped.
    for i in range(3):
        payload, msg_id = csms.expect("TransactionEvent")
        assert payload["eventType"] == "Started", payload
        csms.respond_error(msg_id, "InternalError", "test induced failure")
    h.wait_for("failed 3 of 3 times. Dropping")

    # The queue continues with the next event.
    payload, msg_id = csms.expect("TransactionEvent")
    assert payload["eventType"] == "Updated", payload
    csms.respond(msg_id, {})

    h.send(f"tag {rfid}")
    h.wait_for("Stopping transaction")
    payload, msg_id = csms.expect("TransactionEvent")
    assert payload["eventType"] == "Ended", payload
    csms.respond(msg_id, {})


def test_reset_on_idle_waits_for_transaction(csms, hosts, rfid):
    h = hosts.start(csms.url, "tfocpp-reset-onidle")
    csms.wait_connected()
    h.wait_for("Boot notification accepted", timeout=20)

    h.send(f"tag {rfid}")
    payload, msg_id = csms.expect("Authorize")
    csms.respond(msg_id, {"idTokenInfo": {"status": "Accepted"}})
    h.send("plug")
    h.wait_for(STARTING)
    payload, msg_id = csms.expect("TransactionEvent")
    csms.respond(msg_id, {})

    res = csms.call("Reset", {"type": "OnIdle"})
    assert res["status"] == "Scheduled", res

    # No new transactions while the reset is pending, but the running one
    # continues until it is stopped locally.
    time.sleep(1)
    assert h.count("Resetting") == 0

    h.send(f"tag {rfid}")
    h.wait_for("Stopping transaction")
    while True:
        payload, msg_id = csms.expect("TransactionEvent")
        csms.respond(msg_id, {})
        if payload["eventType"] == "Ended":
            break

    h.wait_for("Resetting")
    h.wait_for("Reset requested")


def test_reset_immediate_drains_the_event_queue(csms, hosts, rfid):
    h = hosts.start(csms.url, "tfocpp-reset-imm")
    csms.wait_connected()
    h.wait_for("Boot notification accepted", timeout=20)

    h.send(f"tag {rfid}")
    payload, msg_id = csms.expect("Authorize")
    csms.respond(msg_id, {"idTokenInfo": {"status": "Accepted"}})
    h.send("plug")
    h.wait_for(STARTING)
    payload, msg_id = csms.expect("TransactionEvent")
    csms.respond(msg_id, {})

    res = csms.call("Reset", {"type": "Immediate"})
    assert res["status"] == "Accepted", res
    h.wait_for("Stopping transaction")

    # The reset happens after the Ended event was delivered.
    while True:
        payload, msg_id = csms.expect("TransactionEvent")
        csms.respond(msg_id, {})
        if payload["eventType"] == "Ended":
            assert payload["transactionInfo"].get("stoppedReason") == "ImmediateReset", payload
            break
    h.wait_for("Resetting")
