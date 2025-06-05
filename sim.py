#!/usr/bin/env python3
# sim.py
#
# Емулятор ESP32-контролера системи освітлення
# ▸ Публікує телеметрію у  BKR/LPNU/Lighting/Status
# ▸ Приймає             →  BKR/LPNU/Lighting/Cmd  ({"target_lux": <float>})
# ▸ Приймає URL прошивки→  BKR/LPNU/Lighting/OTA  (ігнорується у симуляторі)
#
# Запуск:  python sim.py
# Залежності: paho-mqtt ≥ 2.0, numpy

import json
import random
import time
import warnings
from datetime import datetime, timezone

import numpy as np
import paho.mqtt.client as mqtt

BROKER_HOST = "broker.hivemq.com"
BROKER_PORT = 1883
TOPIC_STATUS = "BKR/LPNU/Lighting/Status"
TOPIC_CMD    = "BKR/LPNU/Lighting/Cmd"
TOPIC_OTA    = "BKR/LPNU/Lighting/OTA"
PUBLISH_INTERVAL = 1.0          # cекунди

class LightingSimulator:
    def __init__(self,
                 broker_host=BROKER_HOST,
                 broker_port=BROKER_PORT,
                 interval=PUBLISH_INTERVAL):
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.interval    = interval

        # --- модель пристрою ---
        self.target_lux    = 400.0        # цільова освітленість
        self.presence      = True
        self.duty_cycle    = 0.0          # 0…1
        self.led_max_lux   = 800.0        # 100 % ШІМ
        self.ambient_noise = 20.0         # постійний фон
        self.room_temp_c   = 25.0
        self.rng           = np.random.default_rng(0xBEEF)

        # --- MQTT-клієнт ---
        warnings.filterwarnings("ignore", category=DeprecationWarning)
        self.client = mqtt.Client(client_id=f"sim-light-{random.randint(0,9999)}")
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message

    # ───────────────────────── MQTT callbacks ──────────────────────────────
    def _on_connect(self, client, userdata, flags, rc, *_):
        if rc == 0:
            print(f"[INFO] Connected to {self.broker_host}:{self.broker_port}")
            client.subscribe([(TOPIC_CMD, 1), (TOPIC_OTA, 1)])
        else:
            print(f"[ERROR] MQTT connect failed (rc={rc})")

    def _on_message(self, client, userdata, msg):
        try:
            if msg.topic == TOPIC_CMD:
                data = json.loads(msg.payload.decode())
                if "target_lux" in data:
                    self.target_lux = float(data["target_lux"])
                    print(f"[CMD] target_lux → {self.target_lux}")
            elif msg.topic == TOPIC_OTA:
                print(f"[OTA] URL received: {msg.payload.decode()} (ignored)")
        except Exception as e:
            print("[WARN] Msg parse error:", e)

    # ───────────────────────── оновлення моделі ────────────────────────────
    def _step_model(self):
        self.presence = self.rng.random() < 0.7
        tgt = self.target_lux if self.presence else self.target_lux * 0.2

        measured_lux = self.duty_cycle * self.led_max_lux + self.ambient_noise
        error = tgt - measured_lux
        self.duty_cycle = np.clip(self.duty_cycle + 0.002 * error, 0.0, 1.0)

        noisy_lux = measured_lux + self.rng.normal(0, 5)
        temperature = self.room_temp_c + self.rng.normal(0, 0.3)

        return {
            "lux": round(noisy_lux, 1),
            "temp": round(temperature, 1),
            "presence": self.presence,
            "duty": round(self.duty_cycle, 3),
            "ts": datetime.now(timezone.utc).isoformat()
        }

    # ───────────────────────── головний цикл ───────────────────────────────
    def start(self):
        self.client.connect(self.broker_host, self.broker_port, keepalive=60)
        self.client.loop_start()

        try:
            while True:
                payload = self._step_model()
                self.client.publish(TOPIC_STATUS, json.dumps(payload), qos=1)
                print(f"[PUB] {payload['ts']} | lux={payload['lux']}  "
                      f"temp={payload['temp']}°C  duty={payload['duty']:.2f}")
                time.sleep(self.interval)

        except KeyboardInterrupt:
            print("\n[INFO] Simulator stopped by user")
        finally:
            self.client.loop_stop()
            self.client.disconnect()

if __name__ == "__main__":
    LightingSimulator().start()
