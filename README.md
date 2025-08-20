# ESP32 Portable IoT Weather Station

A portable, battery-friendly weather station built with **ESP32** and **DHT22**, running as a **Wi-Fi Access Point** with a modern web interface. No internet required — connect directly to the ESP32 AP and view live data.

---

## ✨ Features

- **Access Point Web Server**
  - Runs as standalone AP (default SSID: `ESP32-Weather`, password: `weather123`)
  - AP auto-restarts if it drops (AP Guard)
- **Sensor**
  - DHT22 connected to GPIO 4 (configurable)
  - Reads every **10 seconds**
  - Computes **Heat Index (Steadman)**
- **Modern Web UI**
  - Responsive design (mobile + desktop)
  - Large centered KPIs for Temperature, Humidity, Heat Index
  - System stats: Chip, Cores, Revision, CPU MHz, Flash, Heap (in **KB**)
  - Sensor status indicator (`ONLINE` / `OFFLINE`)
  - Built-in **Serial Monitor** view with **Clear** button
- **Robustness**
  - Hardware watchdog using ESP-IDF v5 API
  - Non-blocking loop (`millis()`-based timers, minimal `delay`)
  - Periodic AP keep-alive

---

## 🛠 Hardware

- **Board:** ESP32 (tested on LOLIN32 v1.0.0)
- **Sensor:** DHT22
- **Connections:**
  - DHT22 VCC → 3.3V
  - DHT22 GND → GND
  - DHT22 DATA → GPIO 4 (default)

Optional: Li-Ion/Li-Po battery (many LOLIN boards support charging directly).

---

## 🚀 Build & Upload

1. Clone this repo or download the `.ino` sketch.
2. Open in **Arduino IDE**.
3. Select **Board:** `LOLIN32` or `ESP32 Dev Module`.
4. Select correct **Port**.
5. Upload.  
   (Press **BOOT** during upload if required by your board.)
6. Open **Serial Monitor** (115200 baud) to see simple DHT22 status logs.

---

## 🌐 Usage

1. Connect to Wi-Fi AP:  
   - **SSID:** `ESP32-Weather`  
   - **Password:** `weather123`
2. Open browser at **http://192.168.4.1/**.
3. UI will display:
   - Temperature, Humidity, Heat Index (large centered cards)
   - System stats (chip, cores, revision, flash, heap total/free/used in KB)
   - Sensor status pill
   - Serial Monitor logs with Clear button
