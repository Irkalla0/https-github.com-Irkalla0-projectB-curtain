from __future__ import annotations

import argparse
import json
import queue
import time
import uuid
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import requests
import sounddevice as sd
import yaml
from vosk import KaldiRecognizer, Model

from command_parser import VoiceCommand, parse_voice_command


@dataclass
class GatewayConfig:
    base_url: str
    curtain_id: str
    token: str
    verify_tls: bool
    request_timeout_sec: float


@dataclass
class AsrConfig:
    model_path: str
    sample_rate: int
    device: int | None
    min_confidence: float


@dataclass
class ControlConfig:
    command_cooldown_sec: float
    print_partial: bool


@dataclass
class AppConfig:
    gateway: GatewayConfig
    asr: AsrConfig
    control: ControlConfig


def load_config(path: Path) -> AppConfig:
    raw: dict[str, Any] = yaml.safe_load(path.read_text(encoding="utf-8"))

    gateway = GatewayConfig(
        base_url=str(raw["gateway"]["base_url"]).rstrip("/"),
        curtain_id=str(raw["gateway"]["curtain_id"]),
        token=str(raw["gateway"].get("token", "")),
        verify_tls=bool(raw["gateway"].get("verify_tls", True)),
        request_timeout_sec=float(raw["gateway"].get("request_timeout_sec", 5)),
    )
    asr = AsrConfig(
        model_path=str(raw["asr"]["model_path"]),
        sample_rate=int(raw["asr"].get("sample_rate", 16000)),
        device=raw["asr"].get("device"),
        min_confidence=float(raw["asr"].get("min_confidence", 0.65)),
    )
    control = ControlConfig(
        command_cooldown_sec=float(raw["control"].get("command_cooldown_sec", 2.0)),
        print_partial=bool(raw["control"].get("print_partial", False)),
    )
    return AppConfig(gateway=gateway, asr=asr, control=control)


class CurtainGatewayClient:
    def __init__(self, cfg: GatewayConfig) -> None:
        self._cfg = cfg

    def _headers(self) -> dict[str, str]:
        headers = {"Content-Type": "application/json"}
        if self._cfg.token:
            headers["Authorization"] = f"Bearer {self._cfg.token}"
        return headers

    def set_target(self, target_pos: int) -> requests.Response:
        url = f"{self._cfg.base_url}/api/v1/curtains/{self._cfg.curtain_id}/target"
        body = {"target_pos": int(target_pos), "trace_id": str(uuid.uuid4())}
        return requests.post(
            url,
            json=body,
            headers=self._headers(),
            timeout=self._cfg.request_timeout_sec,
            verify=self._cfg.verify_tls,
        )

    def stop(self) -> requests.Response:
        url = f"{self._cfg.base_url}/api/v1/curtains/{self._cfg.curtain_id}/stop"
        body = {"reason": "voice_command", "trace_id": str(uuid.uuid4())}
        return requests.post(
            url,
            json=body,
            headers=self._headers(),
            timeout=self._cfg.request_timeout_sec,
            verify=self._cfg.verify_tls,
        )


def average_confidence(vosk_result: dict[str, Any]) -> float:
    words = vosk_result.get("result") or []
    if not words:
        return 0.0
    conf_sum = 0.0
    for item in words:
        conf_sum += float(item.get("conf", 0.0))
    return conf_sum / len(words)


def execute_command(client: CurtainGatewayClient, cmd: VoiceCommand) -> bool:
    try:
        if cmd.kind == "stop":
            resp = client.stop()
        else:
            assert cmd.target_pos is not None
            resp = client.set_target(cmd.target_pos)
        ok = 200 <= resp.status_code < 300
        if ok:
            print(f"[OK] cmd={cmd.kind} target={cmd.target_pos} status={resp.status_code}")
        else:
            print(f"[FAIL] cmd={cmd.kind} target={cmd.target_pos} status={resp.status_code} body={resp.text}")
        return ok
    except requests.RequestException as exc:
        print(f"[ERROR] request failed: {exc}")
        return False


def run_voice_loop(cfg: AppConfig) -> None:
    model_path = Path(cfg.asr.model_path)
    if not model_path.exists():
        raise FileNotFoundError(f"Vosk model path not found: {model_path}")

    model = Model(str(model_path))
    rec = KaldiRecognizer(model, cfg.asr.sample_rate)
    rec.SetWords(True)

    client = CurtainGatewayClient(cfg.gateway)
    audio_q: queue.Queue[bytes] = queue.Queue()

    def callback(indata: Any, frames: int, time_info: Any, status: Any) -> None:
        if status:
            print(f"[AUDIO] {status}")
        audio_q.put(bytes(indata))

    print("[INFO] voice loop started. Press Ctrl+C to stop.")
    print(f"[INFO] sample_rate={cfg.asr.sample_rate}, device={cfg.asr.device}")

    last_cmd_at = 0.0
    with sd.RawInputStream(
        samplerate=cfg.asr.sample_rate,
        blocksize=8000,
        device=cfg.asr.device,
        dtype="int16",
        channels=1,
        callback=callback,
    ):
        while True:
            data = audio_q.get()
            if rec.AcceptWaveform(data):
                result = json.loads(rec.Result())
                text = result.get("text", "").strip()
                if not text:
                    continue
                conf = average_confidence(result)
                print(f"[ASR] text={text} conf={conf:.2f}")

                if conf < cfg.asr.min_confidence:
                    print(f"[SKIP] low confidence: {conf:.2f} < {cfg.asr.min_confidence:.2f}")
                    continue

                cmd = parse_voice_command(text)
                if cmd is None:
                    print("[SKIP] no valid command")
                    continue

                now = time.time()
                if now - last_cmd_at < cfg.control.command_cooldown_sec:
                    print("[SKIP] cooldown active")
                    continue

                if execute_command(client, cmd):
                    last_cmd_at = now
            else:
                if cfg.control.print_partial:
                    partial = json.loads(rec.PartialResult()).get("partial", "").strip()
                    if partial:
                        print(f"[PARTIAL] {partial}")


def list_audio_devices() -> None:
    print(sd.query_devices())


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="INMP441 offline voice control for curtain gateway")
    parser.add_argument(
        "--config",
        default="config.yaml",
        help="Path to config file (default: config.yaml)",
    )
    parser.add_argument(
        "--list-devices",
        action="store_true",
        help="List local audio devices and exit",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.list_devices:
        list_audio_devices()
        return
    cfg = load_config(Path(args.config))
    run_voice_loop(cfg)


if __name__ == "__main__":
    main()

