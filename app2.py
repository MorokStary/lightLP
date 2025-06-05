# app.py
import json, queue, threading, time
from datetime import datetime, timezone

import pandas as pd
import paho.mqtt.client as mqtt
from paho.mqtt import publish
import streamlit as st
import plotly.express as px

# ───────────────────── MQTT конфігурація ───────────────────────────────
BROKER, PORT  = "broker.hivemq.com", 1883
TOPIC_DATA    = "BKR/LPNU/Lighting/Status"
TOPIC_CMD     = "BKR/LPNU/Lighting/Cmd"
TOPIC_OTA     = "BKR/LPNU/Lighting/OTA"
HISTORY_LEN   = 500

# ───────────────────── єдина спільна черга  ────────────────────────────
if "data_q" not in globals():          # створюється лише раз у всій програмі
    data_q: queue.Queue = queue.Queue(maxsize=HISTORY_LEN)

# ───────────────────── MQTT колбеки  ────────────────────────────────────
def on_connect(c, u, f, rc):
    if rc == 0:
        c.subscribe(TOPIC_DATA, qos=1)
    else:
        print("MQTT connect failed:", rc)

def on_message(c, u, msg):
    try:
        p = json.loads(msg.payload.decode())
        p["ts"] = datetime.now(timezone.utc)

        try:                     # покладемо в чергу, викинемо старі записи
            data_q.put_nowait(p)
        except queue.Full:
            data_q.get_nowait(); data_q.put_nowait(p)
    except Exception as e:
        print("on_message:", e)

def mqtt_thread():
    cli = mqtt.Client(client_id=f"ui-{time.time_ns()}")
    cli.on_connect, cli.on_message = on_connect, on_message
    cli.connect(BROKER, PORT, keepalive=60)
    cli.loop_forever()

# ───────────────────── запуск MQTT (один раз) ──────────────────────────
if "mqtt_started" not in st.session_state:
    threading.Thread(target=mqtt_thread, daemon=True).start()
    st.session_state.mqtt_started = True

# ───────────────────── ініціалізація session_state ─────────────────────
st.session_state.setdefault("latest",     None)          # останній пакет
st.session_state.setdefault("history_df", pd.DataFrame())

# ───────────────────── читаємо ВСІ свіжі записи з черги ────────────────
has_new = False
while not data_q.empty():
    pkt = data_q.get_nowait()
    has_new = True
    st.session_state.latest = pkt                     # запам’ятали останнє
    # додамо в історію
    st.session_state.history_df = pd.concat(
        [st.session_state.history_df,
         pd.DataFrame([pkt]).set_index("ts")]
    ).tail(HISTORY_LEN)

# якщо немає жодного пакета (перший запуск) — чекаємо, UI не малюємо
if st.session_state.latest is None and not has_new:
    st.set_page_config(page_title="Монітор освітлення")
    st.title("💡 Інтелектуальна система освітлення")
    st.info("⏳ Очікуємо перший пакет телеметрії …")
    st.stop()

# якщо за цей прохід нових пакетів не було — пропускаємо ререндер
if not has_new:
    st.stop()

# ───────────────────── Тепер точно є актуальні дані ────────────────────
l = st.session_state.latest

# ——— UI заголовок
st.set_page_config(page_title="Монітор освітлення", layout="wide")
st.title("💡 Інтелектуальна система освітлення")

# ——— Бічна панель
with st.sidebar:
    st.header("⚙️ Керування")
    target = st.number_input("Цільова освітленість (лк)",
                             50.0, 1000.0, 400.0, 10.0)
    if st.button("Надіслати ціль"):
        publish.single(TOPIC_CMD, json.dumps({"target_lux": target}),
                       hostname=BROKER, port=PORT, qos=1)
        st.success("Ціль надіслана")
    url = st.text_input("URL прошивки для OTA")
    if st.button("Запустити OTA") and url:
        publish.single(TOPIC_OTA, url, hostname=BROKER, port=PORT, qos=1)
        st.warning("OTA ініційовано")

# ——— Метрики
c1, c2, c3, c4 = st.columns(4)
c1.metric("🔆 Освітленість", f"{l['lux']} лк")
c2.metric("🌡 Температура",  f"{l['temp']} °C")
c3.metric("🧍 Присутність",  "Так" if l["presence"] else "Ні")
c4.metric("⚙️ ШІМ",         f"{round(l['duty']*100)} %")
st.caption(f"⏱ Останнє оновлення: {l['ts'].strftime('%H:%M:%S')}")

# ——— Історія
with st.expander("📈 Історія змін"):
    if not st.session_state.history_df.empty:
        fig = px.line(st.session_state.history_df,
                      x=st.session_state.history_df.index,
                      y=["lux", "temp"],
                      labels={"value": "Значення", "ts": "Час"})
        st.plotly_chart(fig, use_container_width=True)
    else:
        st.info("Немає історичних даних.")

# ——— Статус
st.markdown(
    f"<hr><b>Стан мережі:</b> 🟢 Online<br>"
    f"<b>Брокер:</b> {BROKER}<br>"
    f"<b>Топік:</b> {TOPIC_DATA}",
    unsafe_allow_html=True,
)
