from __future__ import annotations

import json
import os
from typing import Callable, Optional


class MQTTSubscriber:
    def __init__(self, host: Optional[str] = None, port: Optional[int] = None) -> None:
        self.host = host or os.getenv("MQTT_HOST")
        self.port = port or int(os.getenv("MQTT_PORT", "1883"))
        self.client = None
        self._handler: Optional[Callable[[str, dict], None]] = None
        if self.host:
            try:
                import paho.mqtt.client as mqtt  # type: ignore
            except Exception:
                self.client = None
            else:
                self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
                self.client.on_message = self._on_message

    # 设置消息处理器
    def set_handler(self, handler: Callable[[str, dict], None]) -> None:
        self._handler = handler

    # 连接到MQTT代理
    def connect(self) -> None:
        if self.client is None or not self.host:
            return
        self.client.connect(self.host, self.port, keepalive=60)
        self.client.loop_start()

    # 订阅指定主题
    def subscribe(self, topic: str, qos: int = 0) -> None:
        if self.client is None:
            return
        self.client.subscribe(topic, qos=qos)

    # 消息处理回调函数
    def _on_message(self, _client, _userdata, message) -> None:
        if self._handler is None:
            return
        try:
            payload = json.loads(message.payload.decode("utf-8"))
        except Exception:
            payload = {"raw": message.payload.decode("utf-8", errors="replace")}
        self._handler(message.topic, payload)

    # 断开与MQTT代理的连接
    def disconnect(self) -> None:
        if self.client is None:
            return
        self.client.loop_stop()
        self.client.disconnect()


subscriber = MQTTSubscriber()
