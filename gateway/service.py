from __future__ import annotations

import secrets
import threading
import time
from dataclasses import dataclass
from typing import Any

from gateway.config import AppConfig
from gateway.mqtt_bridge import MqttBridge
from gateway.protocol import SouthboundFrame, now_unix, sign_frame
from gateway.storage import CurtainStatus, GatewayStorage
from gateway.uart_link import UartError, UartTransport


class GatewayServiceError(Exception):
    pass


@dataclass(frozen=True)
class CommandResult:
    success: bool
    queued: bool
    seq: int
    state: str
    current_pos: int
    fault_code: int
    reason: str = ""


class GatewayService:
    def __init__(self, cfg: AppConfig) -> None:
        self._cfg = cfg
        self._storage = GatewayStorage(cfg.gateway.storage.sqlite_path)
        self._uart = UartTransport(
            port=cfg.gateway.uart.port,
            baudrate=cfg.gateway.uart.baudrate,
            timeout_sec=cfg.gateway.uart.timeout_sec,
            retry_times=cfg.gateway.uart.retry_times,
        )
        self._mqtt = MqttBridge(cfg.mqtt)
        self._seq_lock = threading.Lock()
        self._seq = 0

    def start(self) -> None:
        self._mqtt.start()

    def stop(self) -> None:
        self._mqtt.stop()

    def _next_seq(self) -> int:
        with self._seq_lock:
            self._seq += 1
            return self._seq

    def _build_frame(self, cmd: str, target_pos: int) -> SouthboundFrame:
        seq = self._next_seq()
        timestamp = now_unix()
        nonce = secrets.token_hex(8)
        hmac_hex = sign_frame(
            shared_key=self._cfg.gateway.hmac.shared_key,
            device_id=self._cfg.gateway.device_id,
            cmd=cmd,
            target_pos=target_pos,
            timestamp=timestamp,
            nonce=nonce,
            seq=seq,
        )
        return SouthboundFrame(
            device_id=self._cfg.gateway.device_id,
            cmd=cmd,
            target_pos=target_pos,
            timestamp=timestamp,
            nonce=nonce,
            seq=seq,
            hmac_hex=hmac_hex,
        )

    def _save_status(self, curtain_id: str, state: str, current_pos: int, target_pos: int, fault_code: int) -> None:
        ts = int(time.time())
        self._storage.upsert_status(
            CurtainStatus(
                curtain_id=curtain_id,
                state=state,
                current_pos=current_pos,
                target_pos=target_pos,
                fault_code=fault_code,
                updated_at=ts,
            )
        )
        if fault_code != 0:
            self._storage.append_fault(curtain_id, fault_code, f"fault_code={fault_code}", ts)

    def send_command(
        self,
        actor: str,
        curtain_id: str,
        cmd: str,
        target_pos: int,
        trace_id: str,
    ) -> CommandResult:
        frame = self._build_frame(cmd=cmd, target_pos=target_pos)
        now_ts = int(time.time())
        self._storage.append_audit(
            actor=actor,
            action="api_command",
            detail={
                "curtain_id": curtain_id,
                "cmd": cmd,
                "target_pos": target_pos,
                "seq": frame.seq,
                "trace_id": trace_id,
            },
            ts=now_ts,
        )

        try:
            resp = self._uart.send_frame(frame)
            state = str(resp.get("state", "UNKNOWN"))
            current_pos = int(resp.get("current_pos", target_pos))
            fault_code = int(resp.get("fault_code", 0))
            result = str(resp.get("result", "ok"))
            reason = str(resp.get("reason", ""))
            success = result.lower() == "ok"

            self._save_status(
                curtain_id=curtain_id,
                state=state,
                current_pos=current_pos,
                target_pos=target_pos,
                fault_code=fault_code,
            )
            self._mqtt.publish(
                f"curtain/{curtain_id}/status",
                {
                    "seq": frame.seq,
                    "state": state,
                    "current_pos": current_pos,
                    "target_pos": target_pos,
                    "fault_code": fault_code,
                    "trace_id": trace_id,
                },
            )
            if fault_code != 0:
                self._mqtt.publish(
                    f"curtain/{curtain_id}/faults",
                    {
                        "seq": frame.seq,
                        "fault_code": fault_code,
                        "state": state,
                        "trace_id": trace_id,
                    },
                )
            return CommandResult(
                success=success,
                queued=False,
                seq=frame.seq,
                state=state,
                current_pos=current_pos,
                fault_code=fault_code,
                reason=reason,
            )
        except UartError as exc:
            if not self._cfg.gateway.queue.enable_offline_queue:
                raise GatewayServiceError(f"uart send failed: {exc}") from exc

            self._storage.enqueue_outbound(
                curtain_id=curtain_id,
                cmd=cmd,
                payload={
                    "target_pos": target_pos,
                    "trace_id": trace_id,
                    "reason": str(exc),
                },
                ts=now_ts,
            )
            self._mqtt.publish(
                f"curtain/{curtain_id}/replay",
                {
                    "queued": True,
                    "cmd": cmd,
                    "target_pos": target_pos,
                    "trace_id": trace_id,
                    "reason": str(exc),
                },
            )
            return CommandResult(
                success=False,
                queued=True,
                seq=frame.seq,
                state="QUEUED",
                current_pos=0,
                fault_code=0,
                reason=str(exc),
            )

    def get_status(self, curtain_id: str) -> dict[str, Any]:
        data = self._storage.get_status(curtain_id)
        if data is None:
            return {
                "curtain_id": curtain_id,
                "state": "UNKNOWN",
                "current_pos": 0,
                "target_pos": 0,
                "fault_code": 0,
                "updated_at": 0,
            }
        return {
            "curtain_id": data.curtain_id,
            "state": data.state,
            "current_pos": data.current_pos,
            "target_pos": data.target_pos,
            "fault_code": data.fault_code,
            "updated_at": data.updated_at,
        }

    def list_faults(self, curtain_id: str, limit: int = 50) -> list[dict[str, Any]]:
        return self._storage.list_faults(curtain_id, limit=limit)

    def replay_queue_once(self) -> int:
        queued = self._storage.list_outbound_queue(limit=100)
        replayed = 0
        for item in queued:
            payload = item["payload"]
            if isinstance(payload, str):
                import json

                payload_obj = json.loads(payload)
            else:
                payload_obj = payload
            result = self.send_command(
                actor="queue-replay",
                curtain_id=item["curtain_id"],
                cmd=item["cmd"],
                target_pos=int(payload_obj.get("target_pos", 0)),
                trace_id=str(payload_obj.get("trace_id", "")),
            )
            if result.success:
                self._storage.delete_outbound(int(item["id"]))
                replayed += 1
        return replayed
