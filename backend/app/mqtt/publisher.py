from __future__ import annotations

import json
import os
from typing import Any, Optional


class MQTTPublisher:
    def __init__(self, host: Optional[str] = None, port: Optional[int] = None) -> None:
        self.host = host or os.getenv("MQTT_HOST")
        self.port = port or int(os.getenv("MQTT_PORT", "1883"))
        self.client = None
        if self.host:
            try:
                import paho.mqtt.client as mqtt  # type: ignore
            except Exception:
                self.client = None
            else:
                self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

    def connect(self) -> None:
        if self.client is None or not self.host:
            return
        self.client.connect(self.host, self.port, keepalive=60)
        self.client.loop_start()

    def publish(self, topic: str, payload: dict[str, Any], qos: int = 0) -> None:
        if self.client is None:
            return
        self.client.publish(topic, json.dumps(payload, ensure_ascii=False), qos=qos)

    def publish_telemetry(self, pair_id: str, payload: dict[str, Any]) -> None:
        self.publish(f"leashlink/{pair_id}/telemetry", payload, qos=0)

    def publish_event(self, pair_id: str, payload: dict[str, Any]) -> None:
        self.publish(f"leashlink/{pair_id}/event", payload, qos=1)

    def publish_status(self, pair_id: str, payload: dict[str, Any]) -> None:
        self.publish(f"leashlink/{pair_id}/status", payload, qos=1)

    def publish_cmd_ack(self, pair_id: str, payload: dict[str, Any]) -> None:
        self.publish(f"leashlink/{pair_id}/cmd_ack", payload, qos=1)

    def disconnect(self) -> None:
        if self.client is None:
            return
        self.client.loop_stop()
        self.client.disconnect()


publisher = MQTTPublisher()
