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


class OcppCallError(Exception):
    def __init__(self, code, description, details):
        super().__init__(f"{code}: {description}")
        self.code = code
        self.description = description
        self.details = details


class MiniCsms:
    def __init__(self, certfile=None, keyfile=None, client_ca=None):
        self.requests = queue.Queue()
        self.responses = queue.Queue()
        self.security_events = []
        self.result_errors = []
        self.last_auth = None
        # Answers the next Heartbeat with an invalid payload to provoke a
        # CALLRESULTERROR, the mangled message id is appended to mangled_ids.
        self.mangle_next_heartbeat = False
        self.mangled_ids = []
        self.ws = None
        self.connected = threading.Event()
        self.connection_history = []
        self.connection_condition = threading.Condition()

        ssl_context = None
        host = "127.0.0.1"
        if certfile is not None:
            ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            ssl_context.load_cert_chain(certfile, keyfile)
            if client_ca is not None:
                ssl_context.verify_mode = ssl.CERT_OPTIONAL
                ssl_context.load_verify_locations(client_ca)
            host = "localhost"
        self.ssl_context = ssl_context
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
        peer_certificate = ws.socket.getpeercert(binary_form=True) if self.ssl_context is not None else None
        with self.connection_condition:
            self.connection_history.append(peer_certificate)
            self.connection_condition.notify_all()
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
                        if self.mangle_next_heartbeat:
                            self.mangle_next_heartbeat = False
                            self.mangled_ids.append(msg_id)
                            self.respond(msg_id, {"currentTime": 42})
                        else:
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
                elif msg[0] == 4:
                    self.responses.put((msg[1], OcppCallError(msg[2], msg[3], msg[4])))
                elif msg[0] == 5:
                    self.result_errors.append((msg[1], msg[2], msg[3]))
        except Exception:
            pass
        finally:
            if self.ws is ws:
                self.connected.clear()

    def respond(self, msg_id, payload):
        self.ws.send(json.dumps([3, msg_id, payload]))

    def respond_error(self, msg_id, code, description=""):
        self.ws.send(json.dumps([4, msg_id, code, description, {}]))

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
                if isinstance(rpayload, OcppCallError):
                    raise rpayload
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

    def wait_connection_count(self, count, timeout=15):
        deadline = time.monotonic() + timeout
        with self.connection_condition:
            while len(self.connection_history) < count:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise TimeoutError(f"charge point did not establish connection {count}")
                self.connection_condition.wait(remaining)

    def trust_client_ca(self, cafile):
        if self.ssl_context is None:
            raise RuntimeError("client CAs require TLS")
        self.ssl_context.load_verify_locations(cafile)

    def stop(self):
        self.server.shutdown()
