# Minimal OCPP 2.1 CSMS for testing the M06 OCSP flows.
# Automatic responses only for boot/heartbeat/status, all
# other charge point requests are handed to the test via expect().

import json
import queue
import threading
import time
import uuid

from websockets.sync.server import serve


class MiniCsms:
    def __init__(self):
        self.requests = queue.Queue()
        self.responses = queue.Queue()
        self.ws = None
        self.connected = threading.Event()
        self.server = serve(self._handler, "127.0.0.1", 0, select_subprotocol=lambda conn, protos: "ocpp2.1")
        self.port = self.server.socket.getsockname()[1]
        self.url = f"ws://127.0.0.1:{self.port}"
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()

    def _handler(self, ws):
        self.ws = ws
        self.connected.set()
        try:
            for raw in ws:
                msg = json.loads(raw)
                if msg[0] == 2:
                    _, msg_id, action, payload = msg
                    if action == "BootNotification":
                        self.respond(msg_id, {
                            "currentTime": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                            "interval": 300,
                            "status": "Accepted",
                        })
                    elif action == "Heartbeat":
                        self.respond(msg_id, {"currentTime": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())})
                    elif action in ("StatusNotification", "SecurityEventNotification"):
                        self.respond(msg_id, {})
                    else:
                        self.requests.put((action, payload, msg_id))
                elif msg[0] == 3:
                    self.responses.put((msg[1], msg[2]))
        except Exception:
            pass
        finally:
            self.connected.clear()

    def respond(self, msg_id, payload):
        self.ws.send(json.dumps([3, msg_id, payload]))

    def call(self, action, payload, timeout=10):
        msg_id = str(uuid.uuid4())
        self.ws.send(json.dumps([2, msg_id, action, payload]))
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                rid, rpayload = self.responses.get(timeout=deadline - time.monotonic())
            except queue.Empty:
                break
            if rid == msg_id:
                return rpayload
        raise TimeoutError(f"no response to {action} within {timeout} s")

    def expect(self, action, timeout=15):
        # Returns (payload, msg_id), the test must answer via respond().
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                a, payload, msg_id = self.requests.get(timeout=deadline - time.monotonic())
            except queue.Empty:
                break
            if a == action:
                return payload, msg_id
        raise TimeoutError(f"no {action} request within {timeout} s")

    def disconnect(self):
        if self.ws is not None:
            self.ws.close()

    def wait_connected(self, timeout=15):
        if not self.connected.wait(timeout):
            raise TimeoutError("charge point did not connect")

    def stop(self):
        self.server.shutdown()
