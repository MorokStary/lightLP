import streamlit as st
from streamlit_autorefresh import st_autorefresh
import pandas as pd

from mqtt_handler import GreenhouseMQTTClient, sensor_data

# ─── Start MQTT client in background ───────────────────────────────────────
mqtt_client = GreenhouseMQTTClient()
mqtt_client.start()

# ─── Streamlit page config ─────────────────────────────────────────────────
st.set_page_config(page_title="Smart Greenhouse Dashboard", layout="wide")
st.title("🌿 Розумний моніторинг мікроклімату теплиці")

# ─── Auto-refresh every second ───────────────────────────────────────────────
_ = st_autorefresh(interval=1000, limit=None, key="data_refresh")

# ─── Initialize history in session_state ────────────────────────────────────
if "history" not in st.session_state:
    st.session_state.history = {
        "timestamp":    [],
        "IndoorTemp":   [],
        "IndoorHum":    [],
        "Light":        [],
        "MP1":          [],
        "MP2":          [],
    }

# ─── Append latest reading to history ────────────────────────────────────────
ts = sensor_data.get("timestamp")
if ts:
    st.session_state.history["timestamp"].append(ts)
    for key in ("IndoorTemp", "IndoorHum", "Light", "MP1", "MP2"):
        try:
            st.session_state.history[key].append(float(sensor_data.get(key)))
        except (TypeError, ValueError):
            st.session_state.history[key].append(None)

# ─── Build a DataFrame for plotting ─────────────────────────────────────────
df = pd.DataFrame(st.session_state.history).set_index("timestamp")

# ─── Top row of live metrics ────────────────────────────────────────────────
cols = st.columns(5)
cols[0].metric("🌡️ Temperature (°C)", sensor_data["IndoorTemp"] or "—")
cols[1].metric("💧 Humidity (%)",     sensor_data["IndoorHum"]  or "—")
cols[2].metric("🔆 Light (%)",        sensor_data["Light"]      or "—")
cols[3].metric("🌱 Moisture P1 (%)",   sensor_data["MP1"]        or "—")
cols[4].metric("🌱 Moisture P2 (%)",   sensor_data["MP2"]        or "—")

st.markdown("""---""")

# ─── Relay status & controls ────────────────────────────────────────────────
with st.expander("🔌 Relay Status & Controls", expanded=False):
    st.subheader("Current State")
    r1, r2 = st.columns(2)
    r1.metric("💨 Fan",  sensor_data["Fan"]  or "—")
    r2.metric("💦 Pump", sensor_data["Pump"] or "—")

    st.subheader("Toggle Relays")
    c1, c2 = st.columns(2)
    with c1:
        if st.button("Toggle Fan"):
            new = "OFF" if sensor_data["Fan"] == "ON" else "ON"
            mqtt_client.publish_command("LPNU/BKR/KONON/Fan", new)
    with c2:
        if st.button("Toggle Pump"):
            new = "OFF" if sensor_data["Pump"] == "ON" else "ON"
            mqtt_client.publish_command("LPNU/BKR/KONON/Pump", new)

st.markdown("""---""")

# ─── Historical charts in tabs ───────────────────────────────────────────────
st.subheader("📈 Historical Data")
tab1, tab2 = st.tabs(["Temp & Humidity", "Light & Moisture"])

with tab1:
    if not df[["IndoorTemp", "IndoorHum"]].dropna().empty:
        st.line_chart(df[["IndoorTemp", "IndoorHum"]])
    else:
        st.info("Waiting for temperature & humidity data…")

with tab2:
    if not df[["Light", "MP1", "MP2"]].dropna().empty:
        st.line_chart(df[["Light", "MP1", "MP2"]])
    else:
        st.info("Waiting for light & moisture data…")

st.markdown("""---""")

st.caption(f"🕒 Last update: {ts or '—'}")
