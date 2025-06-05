import json
from datetime import datetime

import paho.mqtt.client as mqtt
import pandas as pd
import streamlit as st
import plotly.express as px
from streamlit_autorefresh import st_autorefresh

BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC_STATUS = "BKR/LPNU/Lighting/Status"
TOPIC_CMD = "BKR/LPNU/Lighting/Cmd"
TOPIC_OTA = "BKR/LPNU/Lighting/OTA"
HISTORY_LEN = 500

# ─── Global shared state ──────────────────────────────────────────────────
sensor_data = {
    "lux": None,
    "temp": None,
    "presence": None,
    "duty": None,
    "timestamp": None,
}

class LightingMQTTClient:
    """Handle MQTT communication and update sensor_data."""

    def __init__(self, broker=BROKER, port=PORT):
        self.broker = broker
        self.port = port
        self.client = mqtt.Client()
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            client.subscribe(TOPIC_STATUS)
        else:
            print(f"[ERROR] MQTT connect failed (rc={rc})")

    def _on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode())
            sensor_data.update(payload)
            sensor_data["timestamp"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        except Exception as e:
            print("[WARN] on_message:", e)

    def start(self):
        try:
            self.client.connect(self.broker, self.port, keepalive=60)
            self.client.loop_start()
            print("[INFO] MQTT loop started")
        except Exception as e:
            print(f"[ERROR] MQTT start failed: {e}")

    def publish_command(self, target):
        self.client.publish(TOPIC_CMD, json.dumps({"target_lux": target}))

    def publish_ota(self, url):
        self.client.publish(TOPIC_OTA, url)


# ─── Start MQTT client once ───────────────────────────────────────────────
if "mqtt_client" not in st.session_state:
    st.session_state.mqtt_client = LightingMQTTClient()
    st.session_state.mqtt_client.start()

# ─── Initialize history ───────────────────────────────────────────────────
if "history" not in st.session_state:
    st.session_state.history = {
        "timestamp": [],
        "lux": [],
        "temp": [],
        "presence": [],
        "duty": [],
    }

# ─── Append latest reading to history ─────────────────────────────────────
ts = sensor_data.get("timestamp")
if ts:
    st.session_state.history["timestamp"].append(ts)
    for key in ("lux", "temp", "presence", "duty"):
        st.session_state.history[key].append(sensor_data.get(key))

    # limit history length
    if len(st.session_state.history["timestamp"]) > HISTORY_LEN:
        for k in st.session_state.history:
            st.session_state.history[k] = st.session_state.history[k][-HISTORY_LEN:]

df = pd.DataFrame(st.session_state.history).set_index("timestamp")

# ─── Page config and auto refresh ─────────────────────────────────────────
st.set_page_config(page_title="Монітор освітлення", layout="wide")
st.title("💡 Інтелектуальна система освітлення")
st_autorefresh(interval=2000, key="refresh")

# ─── Sidebar controls ─────────────────────────────────────────────────────
with st.sidebar:
    st.header("⚙️ Керування")
    target = st.number_input("Цільова освітленість (лк)", 50.0, 1000.0, 400.0, 10.0)
    if st.button("Надіслати ціль"):
        st.session_state.mqtt_client.publish_command(target)
        st.success("Ціль надіслана")
    ota_url = st.text_input("URL прошивки для OTA")
    if st.button("Запустити OTA") and ota_url:
        st.session_state.mqtt_client.publish_ota(ota_url)
        st.warning("OTA ініційовано")

# ─── Current metrics ─────────────────────────────────────────────────────
l = sensor_data

c1, c2, c3, c4 = st.columns(4)
c1.metric("🔆 Освітленість", f"{l['lux']} лк" if l['lux'] is not None else "—")
c2.metric("🌡 Температура", f"{l['temp']} °C" if l['temp'] is not None else "—")
c3.metric("🧍 Присутність", "Так" if l.get("presence") else "Ні")
c4.metric("⚙️ ШІМ", f"{round(l['duty']*100)}%" if l['duty'] is not None else "—")
st.caption(f"⏱ Останнє оновлення: {ts or '—'}")

# ─── History chart ───────────────────────────────────────────────────────
with st.expander("📈 Історія змін"):
    if not df[["lux", "temp"]].dropna().empty:
        fig = px.line(df, x=df.index, y=["lux", "temp"], labels={"value": "Значення", "timestamp": "Час"})
        st.plotly_chart(fig, use_container_width=True)
    else:
        st.info("Немає історичних даних.")

# ─── Status info ─────────────────────────────────────────────────────────
status = "🟢 Online" if l["lux"] is not None else "🔴 Очікування"
st.markdown(
    f"<hr><b>Стан мережі:</b> {status}  "
    f"<br><b>Брокер:</b> {BROKER}  "
    f"<br><b>Топік:</b> {TOPIC_STATUS}",
    unsafe_allow_html=True,
)
