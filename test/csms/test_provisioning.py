# Provisioning against the CSMS: boot, heartbeat, variables, dual instance.

from conftest import WS


def test_boot_profile1(api, stations, hosts):
    name, db_id = stations.create(security_profile=1, password="test-basic-auth-pass-01")
    h = hosts.start(WS, name, "test-basic-auth-pass-01")
    h.wait_for("Connected \\(subprotocol ocpp2.1\\)")
    h.wait_for("Boot notification accepted")


def test_heartbeat_interval(api, stations, hosts):
    name, db_id = stations.create(security_profile=1, password="test-basic-auth-pass-01")
    h = hosts.start(WS, name, "test-basic-auth-pass-01")
    h.wait_for("Boot notification accepted")

    since = api.last_ocpp_log_id(db_id)
    api.set_variable(name, "OCPPCommCtrlr", "HeartbeatInterval", "2")
    result = api.wait_for_response(db_id, "SetVariables", since)
    assert result["setVariableResult"][0]["attributeStatus"] == "Accepted"

    h.wait_for("Received result for Heartbeat", timeout=15, min_count=2)


def test_get_variables(api, stations, hosts):
    name, db_id = stations.create(security_profile=1, password="test-basic-auth-pass-01")
    h = hosts.start(WS, name, "test-basic-auth-pass-01")
    h.wait_for("Boot notification accepted")

    since = api.last_ocpp_log_id(db_id)
    api.get_variables(name, [("OCPPCommCtrlr", "HeartbeatInterval"), ("NoSuchCtrlr", "NoSuchVariable")])
    result = api.wait_for_response(db_id, "GetVariables", since)["getVariableResult"]
    by_variable = {r["variable"]["name"]: r for r in result}
    assert by_variable["HeartbeatInterval"]["attributeStatus"] == "Accepted"
    assert int(by_variable["HeartbeatInterval"]["attributeValue"]) > 0
    assert by_variable["NoSuchVariable"]["attributeStatus"] == "UnknownComponent"


def test_dual_instance(api, stations, hosts):
    name1, _ = stations.create(security_profile=1, password="test-basic-auth-pass-01")
    name2, _ = stations.create(security_profile=1, password="test-basic-auth-pass-02")
    h = hosts.start(WS, name1, "test-basic-auth-pass-01",
                    extra=["--name2", name2, "--pass2", "test-basic-auth-pass-02"])
    h.wait_for("Boot notification accepted", min_count=2)


def test_boot_16(api, stations, hosts, binary16):
    # The 1.6 host simulator has a hardcoded identity. Reuse an existing
    # station of a previous run if the delete failed.
    from host import Host
    try:
        station = api.create_station("warp2-X8D", security_profile=0, protocol="ocpp1.6")
        stations.created.append(station["id"])
    except Exception:
        pass
    h = Host(binary16, [WS], cwd=hosts.workdir)
    hosts.hosts.append(h)
    h.wait_for("Received result for BootNotification", timeout=30)
