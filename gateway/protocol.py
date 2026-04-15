from __future__ import annotations

import hmac
import hashlib
import json
import time
from dataclasses import dataclass
from typing import Any


@dataclass(frozen=True)
class SouthboundFrame:
    device_id: str
    cmd: str
    target_pos: int
    timestamp: int
    nonce: str
    seq: int
    hmac_hex: str


def _canonical_message(device_id: str, cmd: str, target_pos: int, timestamp: int, nonce: str, seq: int) -> str:
    return (
        f"device_id={device_id}|cmd={cmd}|target_pos={target_pos}|"
        f"timestamp={timestamp}|nonce={nonce}|seq={seq}"
    )


def sign_frame(shared_key: str, device_id: str, cmd: str, target_pos: int, timestamp: int, nonce: str, seq: int) -> str:
    msg = _canonical_message(device_id, cmd, target_pos, timestamp, nonce, seq).encode("utf-8")
    key = shared_key.encode("utf-8")
    return hmac.new(key, msg, hashlib.sha256).hexdigest()


def verify_frame_signature(shared_key: str, frame: SouthboundFrame) -> bool:
    expected = sign_frame(
        shared_key=shared_key,
        device_id=frame.device_id,
        cmd=frame.cmd,
        target_pos=frame.target_pos,
        timestamp=frame.timestamp,
        nonce=frame.nonce,
        seq=frame.seq,
    )
    return hmac.compare_digest(expected, frame.hmac_hex)


def frame_to_wire(frame: SouthboundFrame) -> str:
    return (
        f"device_id={frame.device_id}|cmd={frame.cmd}|target_pos={frame.target_pos}|"
        f"timestamp={frame.timestamp}|nonce={frame.nonce}|seq={frame.seq}|hmac={frame.hmac_hex}"
    )


def parse_response_line(raw_line: str) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for part in raw_line.strip().split("|"):
        if "=" not in part:
            continue
        key, value = part.split("=", 1)
        result[key.strip()] = value.strip()
    return result


def now_unix() -> int:
    return int(time.time())


def response_payload(seq: int, result: str, state: str, current_pos: int, fault_code: int, reason: str = "") -> str:
    data = {
        "seq": seq,
        "result": result,
        "state": state,
        "current_pos": current_pos,
        "fault_code": fault_code,
    }
    if reason:
        data["reason"] = reason
    return json.dumps(data, ensure_ascii=False)
