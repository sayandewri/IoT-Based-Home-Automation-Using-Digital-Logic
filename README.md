
# IoT-Based Home Automation Using Digital Logic

This repository contains a complete, ready-to-build project that combines **digital logic** (gate-level decisions) with an **ESP32** for **Wi‑Fi/Bluetooth** remote control. It automates basic home loads like **lights**, **fan**, and **buzzer/alarm** based on **PIR motion**, **ambient light (LDR)**, and **temperature (DHT11)**.

---

## ✨ Features

- Digital-logic rules:
  - **Light = PIR AND Dark**
  - **Fan = (Temp > Threshold)**
  - **Alarm = PIR AND NOT(Authorized)**
- ESP32 with:
  - **MQTT** remote control and status (`/home/room1/light`, `/fan`, `/alarm`).
  - **BLE UART** fallback for local control (Android BLE terminal).
  - Built-in **Web UI** (captive portal style) for LAN control if MQTT not configured.
- **Power optimization**: Light sleep when idle, dynamic radio control (disable BLE when on Wi‑Fi), debounced inputs.
- Safety: Relay driver with opto-isolated modules; AC wiring guidelines.

> ⚠️ **Warning:** Working with mains AC is dangerous. Use certified relay modules and keep low-voltage (3.3V/5V) isolated from AC. If unsure, simulate first and ask a qualified person.

---

## 🧩 Repository Layout

```
firmware/esp32_home_auto/          # Arduino sketch for ESP32 (Wi‑Fi, MQTT, BLE, Web UI)
digital_logic/verilog/              # Gate-level modules + testbench for decision logic
hardware/                           # Wiring notes and ASCII schematics
docs/                               # Diagrams, usage notes
mobile/                             # MQTT topics and example dashboards
.github/workflows/ci.yml            # Lint/build checks (markdown/verilog)
LICENSE
README.md
```

---

## 🛠️ Hardware

- **MCU:** ESP32 DevKitC / NodeMCU-32S
- **Sensors:** PIR (HC-SR501), LDR + resistor divider, DHT11
- **Actuators:** 3 x Relay module (5V opto-isolated) or MOSFET for DC loads, buzzer
- **Power:** 5V adapter (≥2A) + AMS1117-3.3 (or onboard 3.3V from ESP32 board)
- **Manual overrides:** pushbuttons (active-low)

**GPIO Mapping (change in `config.h` if needed):**

| Function         | GPIO |
|------------------|-----:|
| PIR              | 13   |
| LDR (ADC)        | 34   |
| DHT11            | 4    |
| RELAY_LIGHT      | 26   |
| RELAY_FAN        | 27   |
| BUZZER           | 14   |
| BTN_LIGHT        | 33   |
| BTN_FAN          | 25   |

---

## 🔗 MQTT Topics

- Commands: `home/room1/light/cmd` (`ON`/`OFF`/`AUTO`), `home/room1/fan/cmd`, `home/room1/alarm/cmd`
- State:    `home/room1/light/state` (`ON`/`OFF`), `.../fan/state`, `.../alarm/state`
- Telemetry: `home/room1/telemetry` (JSON: temp, hum, light, pir, mode)

> Default broker in code is `test.mosquitto.org` (public). Replace with your private broker in `config.h`.

---

## 📶 Web UI (LAN)

When ESP32 connects to Wi‑Fi, it also hosts a tiny web page at `http://<esp32-ip>/` to toggle loads and view sensor data.

---

## 🔋 Power Optimization

- **Light sleep** when no motion for N seconds (keeps RTC & Wi‑Fi connected).
- Disable **BLE** when Wi‑Fi MQTT is active and stable.
- Use **debounced** GPIO interrupts to wake quickly from idle.
- Adjustable **sensor polling** intervals.

---

## ▶️ Getting Started

1. **Install** Arduino IDE and ESP32 board support (Boards Manager → `esp32 by Espressif Systems`).
2. Install libraries: `PubSubClient`, `DHT sensor library`, `Adafruit Unified Sensor`.
3. Open `firmware/esp32_home_auto/esp32_home_auto.ino`.
4. Copy `config.example.h` to `config.h` and edit Wi‑Fi/MQTT credentials.
5. Select board **ESP32 Dev Module**, set correct COM port, and **Upload**.
6. Open **Serial Monitor @ 115200** to read IP address and logs.
7. Visit `http://<esp32-ip>/` or publish to MQTT topics.

---

## 🧪 Verilog (Digital Logic)

The `digital_logic/verilog` folder contains a synthesizable combinational controller implementing the rules along with a testbench (`tb_controller.v`) you can run on any Verilog simulator (Icarus, ModelSim). Use it to prove the gate-level logic before integrating with hardware.

---

## 📱 Mobile Control Options

- **MQTT Dashboard** (Android) → point to your broker and use listed topics.
- **Blynk**: You can adapt easily—send virtual pins in the `onMqttMessage` hook.
- **BLE UART**: Send `LIGHT ON`, `FAN AUTO`, `ALARM OFF`, etc.

---

## 🧯 Safety Checklist

- Use **opto-isolated relay modules** (JD-Vcc/Vcc separation preferred).
- Keep **mains and logic grounds isolated** as per datasheet.
- Add **flyback diodes** if driving inductive DC loads with MOSFETs.
- Enclose the project in a **non-conductive** case with ventilation.

---

## 📜 License

Author: Sayan Dewri
