from __future__ import annotations

import json
import logging
import os
from typing import Any, Optional

logger = logging.getLogger("uvicorn.error")


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
                self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="leashlink-backend-publisher")
                self.client.on_connect = self._on_connect
                self.client.on_disconnect = self._on_disconnect

    # 连接到MQTT代理
    def connect(self) -> None:
        if self.client is None or not self.host:
            logger.info("MQTT publisher disabled host=%s client_available=%s", self.host or "not configured", self.client is not None)
            return
        logger.info("MQTT publisher connecting host=%s port=%s", self.host, self.port)
        self.client.connect(self.host, self.port, keepalive=60)
        self.client.loop_start()

    # 发布消息到指定主题
    def publish(self, topic: str, payload: dict[str, Any], qos: int = 0) -> None:
        if self.client is None:
            logger.info("MQTT publish skipped topic=%s reason=client unavailable payload=%s", topic, payload)
            return
        logger.info("MQTT publish topic=%s qos=%s payload=%s", topic, qos, payload)
        self.client.publish(topic, json.dumps(payload, ensure_ascii=False), qos=qos)

    def _on_connect(self, _client, _userdata, _flags, reason_code, _properties=None) -> None:
        logger.info("MQTT publisher connected host=%s port=%s reason=%s", self.host, self.port, reason_code)

    def _on_disconnect(self, _client, _userdata, *args) -> None:
        logger.info("MQTT publisher disconnected host=%s port=%s detail=%s", self.host, self.port, args)

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
        logger.info("MQTT publisher disconnecting host=%s port=%s", self.host, self.port)
        self.client.loop_stop()
        self.client.disconnect()


publisher = MQTTPublisher()
