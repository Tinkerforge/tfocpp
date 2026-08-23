# Wrapper around the ocpp21_linux/ocpp16_linux host binaries.

import re
import subprocess
import threading
import time


class Host:
    def __init__(self, binary, args, cwd):
        self.proc = subprocess.Popen(
            [binary] + args,
            cwd=cwd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        self.lines = []
        self.lock = threading.Lock()
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()

    def _read(self):
        for line in self.proc.stdout:
            with self.lock:
                self.lines.append(line.rstrip("\n"))

    def send(self, command):
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()

    def log(self):
        with self.lock:
            return list(self.lines)

    def count(self, pattern):
        return sum(1 for line in self.log() if re.search(pattern, line))

    def wait_for(self, pattern, timeout=20, min_count=1):
        # Returns the first match object once pattern occurred min_count times.
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            matches = [m for line in self.log() if (m := re.search(pattern, line))]
            if len(matches) >= min_count:
                return matches[min_count - 1]
            if self.proc.poll() is not None:
                raise RuntimeError(f"host exited with {self.proc.returncode}, log:\n" + "\n".join(self.log()))
            time.sleep(0.2)
        raise TimeoutError(f"pattern {pattern!r} not seen {min_count} time(s) within {timeout} s, log:\n" + "\n".join(self.log()))

    def stop(self):
        if self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()
