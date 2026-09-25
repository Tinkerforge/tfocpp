# Registration-gated Connector availability notifications.
import time
from datetime import datetime, timezone

import pytest

from minicsms import MiniCsms


@pytest.mark.parametrize("status", ["Pending", "Rejected"])
def test_boot_availability_waits_for_acceptance(hosts, status):
    csms = MiniCsms(manual_boot=True)
    try:
        host = hosts.start(csms.url, "boot-event-test")
        csms.wait_connected()
        boot, mid = csms.expect("BootNotification")
        assert boot["reason"] == "PowerUp"
        host.send("secevent TestBeforeRegistration")
        host.wait_for("queueing security event")
        time.sleep(0.2)
        assert [a for a, _ in csms.received_calls] == ["BootNotification"]
        response = {"status": status, "interval": 1,
                    "currentTime": datetime.now(timezone.utc).isoformat()}
        csms.respond(mid, response)
        _, mid = csms.expect("BootNotification", timeout=5)
        assert [a for a, _ in csms.received_calls] == ["BootNotification", "BootNotification"]
        response.update(status="Accepted", interval=2)
        csms.respond(mid, response)
        host.wait_for("Received result for NotifyEvent")
        assert len(csms.availability_events) == 1
        event = csms.availability_events[0]
        assert event["seqNo"] == 0 and not event.get("tbc", False)
        assert len(event["eventData"]) == 1
        data = event["eventData"][0]
        assert data["component"] == {"name": "Connector", "evse": {"id": 1, "connectorId": 1}}
        assert data["variable"] == {"name": "AvailabilityState"}
        assert data["actualValue"] == "Available"
        assert data["trigger"] == "Alerting"
        assert data["eventNotificationType"] == "HardWiredNotification"
        assert data["severity"] == 8
        assert "variableMonitoringId" not in data
        assert "cleared" not in data
        for stamp in (event["generatedAt"], data["timestamp"]):
            assert abs((datetime.now(timezone.utc) - datetime.fromisoformat(stamp.replace("Z", "+00:00"))).total_seconds()) < 10
        assert [e["type"] for e in csms.security_events] == ["TestBeforeRegistration"]
        host.wait_for("Received result for Heartbeat", timeout=5)
        # A normal reconnect already has registration: no second boot snapshot.
        csms.disconnect()
        csms.wait_connection_count(2, timeout=20)
        time.sleep(0.5)
        assert len(csms.availability_events) == 1
    finally:
        csms.stop()
