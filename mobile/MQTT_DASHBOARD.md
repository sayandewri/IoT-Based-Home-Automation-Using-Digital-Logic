
# Mobile Dashboard (MQTT)

Use any MQTT dashboard app. Create switches publishing to:

- `home/room1/light/cmd` payload: `ON` / `OFF` / `AUTO`
- `home/room1/fan/cmd` payload: `ON` / `OFF` / `AUTO`
- `home/room1/alarm/cmd` payload: `ON` / `OFF` / `AUTO`

Subscribe to:
- `home/room1/light/state`
- `home/room1/fan/state`
- `home/room1/alarm/state`
- `home/room1/telemetry` (JSON)
