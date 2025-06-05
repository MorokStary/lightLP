# app.py
import json, queue, threading, time
from datetime import datetime, timezone

import pandas as pd
import paho.mqtt.client as mqtt
from paho.mqtt import publish
import streamlit as st
import plotly.express as px
from streamlit_autorefresh import st_autorefresh

# ───────────────────── MQTT конфігурація ───────────────────────────────
BROKER      = "broker.hivemq.com"
PORT        = 1883
TOPIC_DATA  = "BKR/LPNU/Lighting/Status"
TOPIC_CMD   = "BKR/LPNU/Lighting/Cmd"
TOPIC_OTA   = "BKR/LPNU/Lighting/OTA"
HISTORY_LEN = 500

# ───────────────────── глобальний стан (потокобезпечний) ───────────────
latest = {"lux": None, "temp": None, "presence": None, "duty": None, "ts": None}
data_q = queue.Queue(maxsize=HISTORY_LEN)

# ───────────────────── MQTT callbacks ───────────────────────────────────
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        client.subscribe(TOPIC_DATA, qos=1)
    else:
        print("MQTT connect failed", rc)

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        payload["ts"] = datetime.now(timezone.utc)

        # оновлюємо global latest
        latest.update(payload)
        print(latest)
        # зберігаємо в чергу історії
        try:
            data_q.put_nowait(payload)
        except queue.Full:
            data_q.get_nowait()
            data_q.put_nowait(payload)
    except Exception as e:
        print("on_message error:", e)

def mqtt_thread():
    c = mqtt.Client(client_id=f"ui-{time.time_ns()}")
    c.on_connect = on_connect
    c.on_message = on_message
    c.connect(BROKER, PORT, keepalive=60)
    c.loop_forever()

# ───────────────────── запускаємо MQTT один раз ────────────────────────
if "mqtt_started" not in st.session_state:
    threading.Thread(target=mqtt_thread, daemon=True).start()
    st.session_state.mqtt_started = True

# ───────────────────── ініціалізація session_state ─────────────────────
if "latest" not in st.session_state:
    st.session_state.latest = latest.copy()
    #print(st.session_state.latest)
if "history_df" not in st.session_state:
    st.session_state.history_df = pd.DataFrame()

# ───────────────────── UI заголовок та автоRefresh ─────────────────────
st.set_page_config(page_title="Монітор освітлення", layout="wide")
st.title("💡 Інтелектуальна система освітлення")
st_autorefresh(interval=2000, key="refresh")

# ───────────────────── копіюємо дані з global → session_state ──────────
#print(latest)
st.session_state.latest.update(latest)  # thread-safe: маленький словник
#print(st.session_state.latest)
items = []
while not data_q.empty():
    items.append(data_q.get_nowait())
if items:
    df_new = pd.DataFrame(items).set_index("ts")
    st.session_state.history_df = pd.concat(
        [st.session_state.history_df, df_new]
    ).tail(HISTORY_LEN)

# ───────────────────── бічна панель керування ──────────────────────────
with st.sidebar:
    st.header("⚙️ Керування")
    target = st.number_input("Цільова освітленість (лк)", 50.0, 1000.0, 400.0, 10.0)
    if st.button("Надіслати ціль"):
        publish.single(TOPIC_CMD, json.dumps({"target_lux": target}),
                       hostname=BROKER, port=PORT, qos=1)
        st.success("Ціль надіслана")
    ota_url = st.text_input("URL прошивки для OTA")
    if st.button("Запустити OTA") and ota_url:
        publish.single(TOPIC_OTA, ota_url, hostname=BROKER, port=PORT, qos=1)
        st.warning("OTA ініційовано")

# ───────────────────── поточні метрики ─────────────────────────────────
l = st.session_state.latest

c1, c2, c3, c4 = st.columns(4)
c1.metric("🔆 Освітленість", f"{l['lux']} лк" if l["lux"] else "—")
c2.metric("🌡 Температура", f"{l['temp']} °C" if l["temp"] else "—")
c3.metric("🧍 Присутність", "Так" if l.get("presence") else "Ні")
c4.metric("⚙️ ШІМ", f"{round(l['duty']*100)}%" if l["duty"] else "—")
st.caption(
    f"⏱ Останнє оновлення: {l['ts'].strftime('%H:%M:%S') if l['ts'] else '—'}"
)

# ───────────────────── історія графік ──────────────────────────────────
with st.expander("📈 Історія змін"):
    if not st.session_state.history_df.empty:
        fig = px.line(
            st.session_state.history_df,
            x=st.session_state.history_df.index,
            y=["lux", "temp"],
            labels={"value": "Значення", "ts": "Час"},
        )
        st.plotly_chart(fig, use_container_width=True)
    else:
        st.info("Немає історичних даних.")

# ───────────────────── статус ──────────────────────────────────────────
status = "🟢 Online" if l["lux"] is not None else "🔴 Очікування"
st.markdown(
    f"<hr><b>Стан мережі:</b> {status}  "
    f"<br><b>Брокер:</b> {BROKER}  "
    f"<br><b>Топік:</b> {TOPIC_DATA}",
    unsafe_allow_html=True,
)