import paho.mqtt.client as mqtt
from datetime import datetime

# ─── Global shared state for Streamlit ───────────────────────────────────────
sensor_data = {
    "IndoorTemp": None,
    "IndoorHum":  None,
    "Light":      None,
    "MP1":        None,
    "MP2":        None,
    "Fan":        None,     # added
    "Pump":       None,     # added
    "timestamp":  None
}

class GreenhouseMQTTClient:
    """
    MQTT handler for greenhouse monitoring system.
    Publishes sensor_data into the shared `sensor_data` dict.
    """

    def __init__(self,
                 broker_host: str = "broker.hivemq.com",
                 broker_port: int = 1883):
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.client = mqtt.Client()

        # include relay topics in the subscription list
        self.topics = [
            "LPNU/BKR/KONON/IndoorTemp",
            "LPNU/BKR/KONON/IndoorHum",
            "LPNU/BKR/KONON/Light",
            "LPNU/BKR/KONON/MP1",
            "LPNU/BKR/KONON/MP2",
            "LPNU/BKR/KONON/Fan",     # added
            "LPNU/BKR/KONON/Pump"     # added
        ]

        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print("[INFO] Connected to MQTT broker.")
            for topic in self.topics:
                client.subscribe(topic)
                print(f"[INFO] Subscribed to topic: {topic}")
        else:
            print(f"[ERROR] Connection failed (rc={rc})")

    def _on_message(self, client, userdata, msg):
        payload   = msg.payload.decode()
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        key       = msg.topic.split("/")[-1]   # e.g. "IndoorTemp", "Fan"
        
        # store the payload in shared state
        sensor_data[key]           = payload
        sensor_data["timestamp"]   = timestamp

        print(f"[DATA] {timestamp} | {key} = {payload}")

    def start(self):
        """
        Connect to the broker and run the loop in a background thread.
        """
        try:
            self.client.connect(self.broker_host,
                                self.broker_port,
                                keepalive=60)
            self.client.loop_start()   # ← non-blocking
            print("[INFO] MQTT loop started in background")
        except Exception as e:
            print(f"[ERROR] MQTT start failed: {e}")

    def publish_command(self, topic: str, message: str):
        """
        Send control commands (e.g. ON/OFF) back to the greenhouse.
        """
        self.client.publish(topic, message)
        print(f"[CMD] Published '{message}' to {topic}")
