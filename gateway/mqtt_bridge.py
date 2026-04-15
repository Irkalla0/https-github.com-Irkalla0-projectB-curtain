from __future__ import annotations

import json
from typing import Any

import paho.mqtt.client as mqtt

from gateway.config import MqttConfig


class MqttBridge:
    def __init__(self, cfg: MqttConfig) -> None:
        self._cfg = cfg
        self._client = mqtt.Client(client_id=cfg.client_id)
        self._connected = False

        if cfg.enabled:
            if cfg.username:
                self._client.username_pw_set(cfg.username, cfg.password)

            if cfg.tls.enabled:
                self._client.tls_set(
                    ca_certs=cfg.tls.cafile or None,
                    certfile=cfg.tls.certfile or None,
                    keyfile=cfg.tls.keyfile or None,
                )

            self._client.on_connect = self._on_connect
            self._client.on_disconnect = self._on_disconnect

    def _on_connect(self, _client: mqtt.Client, _userdata: Any, _flags: Any, rc: int, _properties: Any = None) -> None:
        self._connected = rc == 0

    def _on_disconnect(self, _client: mqtt.Client, _userdata: Any, _rc: int, _properties: Any = None) -> None:
        self._connected = False

    def start(self) -> None:
        if not self._cfg.enabled:
            return
        self._client.connect(self._cfg.host, self._cfg.port, keepalive=30)
        self._client.loop_start()

    def stop(self) -> None:
        if not self._cfg.enabled:
            return
        self._client.loop_stop()
        self._client.disconnect()

    def publish(self, topic: str, payload: dict[str, Any]) -> None:
        if not self._cfg.enabled:
            return
        self._client.publish(topic, json.dumps(payload, ensure_ascii=False), qos=1, retain=False)
