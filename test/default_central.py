import asyncio
import logging
from datetime import datetime, timezone
import sys
import threading
import time

try:
    import websockets
except ModuleNotFoundError:
    print("This example relies on the 'websockets' package.")
    print("Please install it by running: ")
    print()
    print(" $ pip install websockets")
    import sys
    sys.exit(1)

from ocpp.routing import on, after
from ocpp.v16 import ChargePoint as cp
from ocpp.v16.enums import Action, RegistrationStatus
from ocpp.v16 import call_result
from ocpp.messages import unpack, MessageType


def is_valid_transition(before, after):
    states = ["Available",
              "Preparing",
              "Charging",
              "SuspendedEVSE",
              "SuspendedEV",
              "Finishing",
              "Reserved",
              "Unavailable",
              "Faulted"]

    transition = "{}{}".format(chr(ord('A') + states.index(before)),
                               chr(ord('1') + states.index(after)))

    valid_transitions = [                                         # From
              "A2", "A3", "A4", "A5",           "A7", "A8", "A9", # Available
        "B1",       "B3", "B4", "B5", "B6",                 "B9", # Preparing
        "C1",             "C4", "C5", "C6",           "C8", "C9", # Charging
        "D1",       "D3",       "D5", "D6",           "D8", "D9", # SuspendedEV
        "E1",       "E3", "E4",       "E6",           "E8", "E9", # SuspendedEVSE
        "F1", "F2",                                   "F8", "F9", # Finishing

        "G1", "G2",                                   "G8", "G9", # Reserved
        "H1", "H2", "H3", "H4", "H5",                       "H9", # Unavailable
        "I1", "I2", "I3", "I4", "I5", "I6",     "I7", "I8"        # Faulted
     # To A     P     C     S     S     F         R     U     F
     #    v     r     h     u     u     i         e     n     a
     #    a     e     a     s     s     n         s     a     u
     #    i     p     r     p     p     i         e     v     l
     #    l     a     g     e     e     s         r     a     t
     #    a     r     i     n     n     h         v     i     e
     #    b     i     n     d     d     i         e     l     d
     #    l     n     g     e     e     n         d     a
     #    e     g           d     d     g               b
     #                      E     E                     l
     #                      V     V                     e
     #                            S
     #                            E
    ]

    return transition in valid_transitions

def addTester(test):
    def decorator(clazz):
        clazz.test = test
        return clazz
    return decorator

class DefaultChargePoint(cp):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.done = False
        self.received_calls = {}
        self.received_results = {}
        self.received_errors = {}
        self.timescale = 1
        self.datetime_start = datetime.now(timezone.utc).replace(microsecond=0)
        self.time_start = time.time()
        self.libocpp = None

    last_status = {}
    status = {}
    test = None

    def get_datetime(self):
        return (self.datetime_start + (datetime.now(timezone.utc).replace(microsecond=0) - self.datetime_start) * self.timescale)

    def get_time(self):
        return (self.time_start + (time.time() - self.time_start) * self.timescale)

    async def route_message(self, raw_msg):
        await super().route_message(raw_msg)

        # this will succeed because super already unpacks the message once.
        msg = unpack(raw_msg)

        if msg.message_type_id == MessageType.Call:
            if not msg.action in self.received_calls:
                self.received_calls[msg.action] = 0
            self.received_calls[msg.action] += 1


    async def call(self, payload, suppress=False):
        try:
            result = await super().call(payload, suppress)
        except Exception as e:
            if not e.__class__ in self.received_errors:
                self.received_errors[e.__class__] = 0
            self.received_errors[e.__class__] += 1
            return None

        if not result.__class__ in self.received_calls:
            self.received_results[result.__class__] = 0
        self.received_results[result.__class__] += 1

        return result

    @on(Action.BootNotification)
    def on_boot_notification(self, charge_point_vendor: str, charge_point_model: str, **kwargs):
        return call_result.BootNotificationPayload(
            current_time=self.get_datetime().isoformat(),
            interval=10,
            status=RegistrationStatus.accepted
        )

    @on(Action.StatusNotification)
    def on_status_notification(self, connector_id: int, error_code, status, **kwargs):
        self.status.setdefault(connector_id, []).append(status)

        if connector_id in self.last_status:
            self.test.assertTrue(is_valid_transition(self.last_status[connector_id], status))

        self.last_status[connector_id] = status

        if connector_id == 0:
            """
            A Charge Point Connector MAY have any of the 9 statuses as shown in the table above. For
            ConnectorId 0, only a limited set is applicable, namely: Available, Unavailable and Faulted.
            """
            self.test.assertIn(status, ["Available", "Unavailable", "Faulted"])

        """
        ChargePointErrorCode EVCommunicationError SHALL only be used with status Preparing,
        SuspendedEV, SuspendedEVSE and Finishing and be treated as warning.
        """
        if error_code == "EVCommunicationError":
            self.test.assertIn(status, ["Preparing", "SuspendedEV", "SuspendedEVSE", "Finishing"])

        return call_result.StatusNotificationPayload()

    @on(Action.Heartbeat)
    def on_heartbeat(self, **kwargs):
        t = self.get_datetime().isoformat()
        return call_result.HeartbeatPayload(current_time=t)

    @on(Action.FirmwareStatusNotification)
    def on_firmware_status(self, **kwargs):
        return call_result.FirmwareStatusNotificationPayload()

    @on(Action.MeterValues)
    def on_meter_values(self, **kwargs):
        return call_result.MeterValuesPayload()

    @on(Action.Authorize)
    def on_authorize(self, id_tag: str, **kwargs):
        return call_result.AuthorizePayload(id_tag_info={"status": "Accepted"})

    @on(Action.StartTransaction)
    def on_start_transaction(self, connector_id: int, id_tag: str, meter_start: int, timestamp: str, **kwargs):
        return call_result.StartTransactionPayload(transaction_id=1234, id_tag_info={"status": "Accepted"})

    @on(Action.StopTransaction)
    def on_stop_transaction(self, **kwargs):
        return call_result.StopTransactionPayload(id_tag_info={"status": "Accepted"})

active_client = None
async def on_connect(websocket, path, charge_point_type, timescale):
    """ For every new charge point that connects, create a ChargePoint
    instance and start listening for messages.
    """
    try:
        requested_protocols = websocket.request_headers[
            'Sec-WebSocket-Protocol']
    except KeyError:
        logging.error(
            "Client hasn't requested any Subprotocol. Closing Connection"
        )
        return await websocket.close()
    if websocket.subprotocol:
        logging.info("Protocols Matched: %s", websocket.subprotocol)
    else:
        # In the websockets lib if no subprotocols are supported by the
        # client and the server, it proceeds without a subprotocol,
        # so we have to manually close the connection.
        logging.warning('Protocols Mismatched | Expected Subprotocols: %s,'
                        ' but client supports  %s | Closing connection',
                        websocket.available_subprotocols,
                        requested_protocols)
        return await websocket.close()

    charge_point_id = path.strip('/')

    global active_client
    active_client = charge_point_type(charge_point_id, websocket)
    active_client.timescale = timescale

    try:
        await active_client.start()
    except websockets.exceptions.ConnectionClosedError:
        return

server = None
done = False
event_loop = None
async def main(port, charge_point_type, timescale):
    global server
    global done
    global event_loop
    event_loop = asyncio.get_running_loop()

    server = await websockets.serve(
        lambda a, b: on_connect(a, b, charge_point_type, timescale),
        '0.0.0.0',
        port,
        subprotocols=['ocpp1.6']
    )

    logging.info("Server Started listening to new connections...")
    await server.wait_closed()
    done = True


def fire_and_forget(coro):
    threading.Thread(target=lambda: asyncio.run(coro), daemon=True).start()

def run_server(port, charge_point_type, timescale):
    global server
    global done
    global active_client
    global event_loop
    done = False
    active_client = None
    server = None
    event_loop = None
    fire_and_forget(main(port, charge_point_type, timescale))
