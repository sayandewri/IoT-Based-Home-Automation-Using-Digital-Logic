
# Documentation

## Build & Flash
- Arduino IDE → Board: ESP32 Dev Module
- Libraries: PubSubClient, DHT sensor library, Adafruit Unified Sensor, ESP32 BLE Arduino (bundled)
- Copy `config.example.h` → `config.h` and set Wi‑Fi/MQTT.

## MQTT Quick Test
- Using mosquitto:
  - `mosquitto_sub -h <broker> -t 'home/room1/telemetry' -v`
  - `mosquitto_pub -h <broker> -t 'home/room1/light/cmd' -m 'AUTO'`

## Verilog Simulation
- Icarus Verilog:
  - `iverilog -o sim digital_logic/verilog/tb_controller.v digital_logic/verilog/controller.v`
  - `vvp sim`

## Web UI
- Visit `http://<esp32-ip>/` after connecting to router.

## Power Tips
- Increase `motionHoldMs` to reduce on/off toggling.
- Raise `LDR_THRESHOLD` in bright rooms (0..4095).
- Use ESP32 deep sleep only if MQTT reconnects are acceptable.
