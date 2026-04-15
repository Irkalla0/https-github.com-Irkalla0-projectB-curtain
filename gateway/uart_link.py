from __future__ import annotations

import threading
from typing import Any

import serial

from gateway.protocol import SouthboundFrame, frame_to_wire, parse_response_line


class UartError(Exception):
    pass


class UartTransport:
    def __init__(self, port: str, baudrate: int, timeout_sec: float, retry_times: int) -> None:
        self._port = port
        self._baudrate = baudrate
        self._timeout = timeout_sec
        self._retry_times = retry_times
        self._lock = threading.Lock()

    def _open(self) -> serial.Serial:
        return serial.Serial(self._port, self._baudrate, timeout=self._timeout)

    def send_frame(self, frame: SouthboundFrame) -> dict[str, Any]:
        line = frame_to_wire(frame) + "\n"
        last_error: Exception | None = None
        for _ in range(self._retry_times + 1):
            try:
                with self._lock:
                    with self._open() as ser:
                        ser.write(line.encode("utf-8"))
                        ser.flush()
                        # Skip comment/debug lines and wait for a structured key=value response.
                        for _i in range(12):
                            raw = ser.readline().decode("utf-8", errors="ignore").strip()
                            if not raw:
                                continue
                            if raw.startswith("#"):
                                continue
                            data = parse_response_line(raw)
                            if "seq" in data and "result" in data:
                                return data
                        raise UartError("no valid response frame")
            except Exception as exc:  # noqa: BLE001
                last_error = exc
        raise UartError(str(last_error) if last_error else "unknown uart error")
