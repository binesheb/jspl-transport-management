import json

import paho.mqtt.client as mqtt

from .config import settings


class MqttPublisher:
    def __init__(self) -> None:
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        if settings.mqtt_username:
            self.client.username_pw_set(settings.mqtt_username, settings.mqtt_password)
        self.connected = False

    def connect(self) -> None:
        self.client.connect(settings.mqtt_host, settings.mqtt_port, keepalive=30)
        self.client.loop_start()
        self.connected = True

    def publish(self, topic: str, payload: dict) -> None:
        if not self.connected:
            return
        self.client.publish(topic, json.dumps(payload), qos=1)


publisher = MqttPublisher()
