
// Copy to config.h and edit your credentials
#pragma once

// Wi‑Fi
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASS "YourWiFiPassword"

// MQTT
#define MQTT_BROKER "test.mosquitto.org"
#define MQTT_PORT 1883

// Topics
#define MQTT_LIGHT_CMD   "home/room1/light/cmd"
#define MQTT_FAN_CMD     "home/room1/fan/cmd"
#define MQTT_ALARM_CMD   "home/room1/alarm/cmd"
#define MQTT_LIGHT_STATE "home/room1/light/state"
#define MQTT_FAN_STATE   "home/room1/fan/state"
#define MQTT_ALARM_STATE "home/room1/alarm/state"
#define MQTT_LIGHT_MODE  "home/room1/light/mode"
#define MQTT_FAN_MODE    "home/room1/fan/mode"
#define MQTT_ALARM_MODE  "home/room1/alarm/mode"
#define MQTT_TELEMETRY   "home/room1/telemetry"
