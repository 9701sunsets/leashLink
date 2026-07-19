from __future__ import annotations

import json
import logging
import os
from typing import Callable, Optional

logger = logging.getLogger("uvicorn.error")


class MQTTSubscriber:
    def __init__(self, host: Optional[str] = None, port: Optional[int] = None) -> None:
        self.host = host or os.getenv("MQTT_HOST")
        self.port = port or int(os.getenv("MQTT_PORT", "1883"))
        self.client = None
        self._handler: Optional[Callable[[str, dict], None]] = None
        self._subscriptions: list[tuple[str, int]] = []
        if self.host:
            try:
                import paho.mqtt.client as mqtt  # type: ignore
            except Exception:
                self.client = None
            else:
                self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="leashlink-backend-subscriber")
                self.client.on_connect = self._on_connect
                self.client.on_disconnect = self._on_disconnect
                self.client.on_message = self._on_message
                self.client.on_subscribe = self._on_subscribe

    # 设置消息处理器
    def set_handler(self, handler: Callable[[str, dict], None]) -> None:
        self._handler = handler

    # 连接到MQTT代理
    def connect(self) -> None:
        if self.client is None or not self.host:
            logger.info("MQTT subscriber disabled host=%s client_available=%s", self.host or "not configured", self.client is not None)
            return
        logger.info("MQTT subscriber connecting host=%s port=%s", self.host, self.port)
        self.client.connect(self.host, self.port, keepalive=60)
        self.client.loop_start()

    # 订阅指定主题
    def subscribe(self, topic: str, qos: int = 0) -> None:
        if (topic, qos) not in self._subscriptions:
            self._subscriptions.append((topic, qos))
        if self.client is None:
            logger.info("MQTT subscribe skipped topic=%s reason=client unavailable", topic)
            return
        result, mid = self.client.subscribe(topic, qos=qos)
        logger.info("MQTT subscribe topic=%s qos=%s result=%s mid=%s", topic, qos, result, mid)

    def _on_connect(self, _client, _userdata, _flags, reason_code, _properties=None) -> None:
        logger.info("MQTT subscriber connected host=%s port=%s reason=%s", self.host, self.port, reason_code)
        for topic, qos in self._subscriptions:
            result, mid = self.client.subscribe(topic, qos=qos)
            logger.info("MQTT resubscribe topic=%s qos=%s result=%s mid=%s", topic, qos, result, mid)

    def _on_disconnect(self, _client, _userdata, *args) -> None:
        logger.info("MQTT subscriber disconnected host=%s port=%s detail=%s", self.host, self.port, args)

    def _on_subscribe(self, _client, _userdata, mid, reason_codes, _properties=None) -> None:
        logger.info("MQTT subscribed mid=%s reason_codes=%s", mid, reason_codes)

    # 消息处理回调函数
    def _on_message(self, _client, _userdata, message) -> None:
        try:
            payload = json.loads(message.payload.decode("utf-8"))
        except Exception:
            payload = {"raw": message.payload.decode("utf-8", errors="replace")}
        logger.info("MQTT received topic=%s payload=%s", message.topic, payload)
        if self._handler is None:
            logger.info("MQTT message ignored topic=%s reason=no handler", message.topic)
            return
        self._handler(message.topic, payload)

    # 断开与MQTT代理的连接
    def disconnect(self) -> None:
        if self.client is None:
            return
        logger.info("MQTT subscriber disconnecting host=%s port=%s", self.host, self.port)
        self.client.loop_stop()
        self.client.disconnect()


subscriber = MQTTSubscriber()
