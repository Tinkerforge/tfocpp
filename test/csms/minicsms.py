# Minimal OCPP 2.1 CSMS for the flows evtivity can not drive (M06 OCSP,
# A05 network profiles). Automatic responses only for boot/heartbeat/
# status/security events, all other charge point requests are handed to
# the test via expect().

import json
import queue
import ssl
import threading
import time
import uuid

from websockets.sync.server import serve


class MiniCsms:
    def __init__(self, certfile=None, keyfile=None):
        self.requests = queue.Queue()
        self.responses = queue.Queue()
        self.security_events = []
        self.last_auth = None
        self.ws = None
        self.connected = threading.Event()

        ssl_context = None
        host = "127.0.0.1"
        if certfile is not None:
            ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            ssl_context.load_cert_chain(certfile, keyfile)
            host = "localhost"
        self.server = serve(self._handler, host, 0, ssl=ssl_context,
                            select_subprotocol=lambda conn, protos: "ocpp2.1")
        self.port = self.server.socket.getsockname()[1]
        scheme = "ws" if ssl_context is None else "wss"
        self.url = f"{scheme}://{host}:{self.port}"
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()

    def _handler(self, ws):
        self.ws = ws
        self.last_auth = ws.request.headers.get("Authorization")
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
                    elif action == "SecurityEventNotification":
                        self.security_events.append(payload)
                        self.respond(msg_id, {})
                    elif action == "StatusNotification":
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
