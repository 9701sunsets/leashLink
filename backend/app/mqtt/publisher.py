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

    # 连接到MQTT代理
    def connect(self) -> None:
        if self.client is None or not self.host:
            return
        self.client.connect(self.host, self.port, keepalive=60)
        self.client.loop_start()

    # 发布消息到指定主题
    def publish(self, topic: str, payload: dict[str, Any], qos: int = 0) -> None:
        if self.client is None:
            return
        self.client.publish(topic, json.dumps(payload, ensure_ascii=False), qos=qos)

    # 设备遥测数据发布服务模块
    def publish_telemetry(self, pair_id: str, payload: dict[str, Any]) -> None:
        self.publish(f"leashlink/{pair_id}/telemetry", payload, qos=0)

    # 事件发布服务模块
    def publish_event(self, pair_id: str, payload: dict[str, Any]) -> None:
        self.publish(f"leashlink/{pair_id}/event", payload, qos=1)

    # 设备状态发布服务模块
    def publish_status(self, pair_id: str, payload: dict[str, Any]) -> None:
        self.publish(f"leashlink/{pair_id}/status", payload, qos=1)

    # 设备命令应答发布服务模块
    def publish_cmd_ack(self, pair_id: str, payload: dict[str, Any]) -> None:
        self.publish(f"leashlink/{pair_id}/cmd_ack", payload, qos=1)

    # 断开与MQTT代理的连接
    def disconnect(self) -> None:
        if self.client is None:
            return
        self.client.loop_stop()
        self.client.disconnect()


publisher = MQTTPublisher()
