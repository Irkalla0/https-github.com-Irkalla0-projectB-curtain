from __future__ import annotations

from gateway.protocol import SouthboundFrame, frame_to_wire, parse_response_line, sign_frame, verify_frame_signature


def test_sign_and_verify() -> None:
    key = "demo_key"
    frame = SouthboundFrame(
        device_id="curtain-node-001",
        cmd="target",
        target_pos=50,
        timestamp=1710000000,
        nonce="abcdef0123456789",
        seq=12,
        hmac_hex=sign_frame(
            shared_key=key,
            device_id="curtain-node-001",
            cmd="target",
            target_pos=50,
            timestamp=1710000000,
            nonce="abcdef0123456789",
            seq=12,
        ),
    )
    assert verify_frame_signature(key, frame)


def test_wire_format_and_response_parse() -> None:
    frame = SouthboundFrame(
        device_id="curtain-node-001",
        cmd="stop",
        target_pos=0,
        timestamp=1710000001,
        nonce="n1",
        seq=2,
        hmac_hex="abc",
    )
    line = frame_to_wire(frame)
    assert "cmd=stop" in line

    resp = parse_response_line("seq=2|result=ok|state=IDLE|current_pos=40|fault_code=0")
    assert resp["result"] == "ok"
    assert resp["state"] == "IDLE"
