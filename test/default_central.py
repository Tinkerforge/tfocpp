import asyncio
import logging
from datetime import datetime
import sys
import threading

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

class DefaultChargePoint(cp):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.done = False
        self.received_calls = {}
        self.received_results = {}
        self.received_errors = {}
        self.timescale = 1
        self.time_start = datetime.utcnow()

    def get_datetime(self):
        return (self.time_start + (datetime.utcnow() - self.time_start) * self.timescale)

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
    def on_status_notification(self, connector_id: int, **kwargs):
        return call_result.StatusNotificationPayload()

    @on(Action.Heartbeat)
    def on_heartbeat(self, **kwargs):
        t = self.get_datetime().isoformat()
        return call_result.HeartbeatPayload(current_time=t + "Z")

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
