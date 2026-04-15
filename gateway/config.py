from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any

import yaml


@dataclass(frozen=True)
class TlsServerConfig:
    enabled: bool
    certfile: str
    keyfile: str
    cafile: str
    require_client_cert: bool


@dataclass(frozen=True)
class ServerConfig:
    host: str
    port: int
    tls: TlsServerConfig


@dataclass(frozen=True)
class AuthConfig:
    jwt_secret: str
    jwt_algorithms: list[str]
    static_tokens: list[str]


@dataclass(frozen=True)
class UartConfig:
    port: str
    baudrate: int
    timeout_sec: float
    retry_times: int


@dataclass(frozen=True)
class HmacConfig:
    shared_key: str
    timestamp_window_sec: int


@dataclass(frozen=True)
class StorageConfig:
    sqlite_path: str


@dataclass(frozen=True)
class QueueConfig:
    enable_offline_queue: bool


@dataclass(frozen=True)
class GatewayConfig:
    default_curtain_id: str
    device_id: str
    uart: UartConfig
    hmac: HmacConfig
    storage: StorageConfig
    queue: QueueConfig


@dataclass(frozen=True)
class MqttTlsConfig:
    enabled: bool
    cafile: str
    certfile: str
    keyfile: str


@dataclass(frozen=True)
class MqttConfig:
    enabled: bool
    host: str
    port: int
    client_id: str
    username: str
    password: str
    tls: MqttTlsConfig


@dataclass(frozen=True)
class AppConfig:
    server: ServerConfig
    auth: AuthConfig
    gateway: GatewayConfig
    mqtt: MqttConfig


def _as_bool(raw: Any, default: bool = False) -> bool:
    if raw is None:
        return default
    return bool(raw)


def load_config(path: str | Path) -> AppConfig:
    cfg_path = Path(path)
    raw: dict[str, Any] = yaml.safe_load(cfg_path.read_text(encoding="utf-8"))

    server_tls = raw["server"]["tls"]
    server = ServerConfig(
        host=str(raw["server"].get("host", "0.0.0.0")),
        port=int(raw["server"].get("port", 8443)),
        tls=TlsServerConfig(
            enabled=_as_bool(server_tls.get("enabled", True), True),
            certfile=str(server_tls.get("certfile", "")),
            keyfile=str(server_tls.get("keyfile", "")),
            cafile=str(server_tls.get("cafile", "")),
            require_client_cert=_as_bool(server_tls.get("require_client_cert", False), False),
        ),
    )

    auth = AuthConfig(
        jwt_secret=str(raw["auth"].get("jwt_secret", "")),
        jwt_algorithms=[str(item) for item in raw["auth"].get("jwt_algorithms", ["HS256"])],
        static_tokens=[str(item) for item in raw["auth"].get("static_tokens", [])],
    )

    gw_raw = raw["gateway"]
    uart_raw = gw_raw["uart"]
    hmac_raw = gw_raw["hmac"]
    storage_raw = gw_raw["storage"]
    queue_raw = gw_raw.get("queue", {})
    gateway = GatewayConfig(
        default_curtain_id=str(gw_raw.get("default_curtain_id", "living-room")),
        device_id=str(gw_raw["device_id"]),
        uart=UartConfig(
            port=str(uart_raw["port"]),
            baudrate=int(uart_raw.get("baudrate", 115200)),
            timeout_sec=float(uart_raw.get("timeout_sec", 0.8)),
            retry_times=int(uart_raw.get("retry_times", 2)),
        ),
        hmac=HmacConfig(
            shared_key=str(hmac_raw["shared_key"]),
            timestamp_window_sec=int(hmac_raw.get("timestamp_window_sec", 120)),
        ),
        storage=StorageConfig(sqlite_path=str(storage_raw["sqlite_path"])),
        queue=QueueConfig(enable_offline_queue=_as_bool(queue_raw.get("enable_offline_queue", True), True)),
    )

    mqtt_raw = raw.get("mqtt", {})
    mqtt_tls_raw = mqtt_raw.get("tls", {})
    mqtt = MqttConfig(
        enabled=_as_bool(mqtt_raw.get("enabled", False), False),
        host=str(mqtt_raw.get("host", "127.0.0.1")),
        port=int(mqtt_raw.get("port", 8883)),
        client_id=str(mqtt_raw.get("client_id", "projectb-gateway")),
        username=str(mqtt_raw.get("username", "")),
        password=str(mqtt_raw.get("password", "")),
        tls=MqttTlsConfig(
            enabled=_as_bool(mqtt_tls_raw.get("enabled", True), True),
            cafile=str(mqtt_tls_raw.get("cafile", "")),
            certfile=str(mqtt_tls_raw.get("certfile", "")),
            keyfile=str(mqtt_tls_raw.get("keyfile", "")),
        ),
    )

    return AppConfig(server=server, auth=auth, gateway=gateway, mqtt=mqtt)
