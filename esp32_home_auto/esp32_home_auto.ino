
/*
  ESP32 Home Automation (Digital Logic + IoT)
  - Sensors: PIR (GPIO 13), LDR (ADC1_CH6 GPIO 34), DHT11 (GPIO 4)
  - Actuators: RELAY_LIGHT (GPIO 26), RELAY_FAN (GPIO 27), BUZZER (GPIO 14)
  - Buttons: BTN_LIGHT (GPIO 33), BTN_FAN (GPIO 25)
  - MQTT topics and Web UI for control
  - BLE UART fallback
  - Power optimization with light sleep
*/

#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "config.h"

#define DHTPIN 4
#define DHTTYPE DHT11

// GPIOs
const int PIR_PIN = 13;
const int LDR_PIN = 34; // ADC
const int RELAY_LIGHT = 26;
const int RELAY_FAN = 27;
const int BUZZER = 14;
const int BTN_LIGHT = 33;
const int BTN_FAN = 25;

// Thresholds
int LDR_THRESHOLD = 1800; // adjust for darkness (0-4095)
float TEMP_THRESHOLD = 28.0;

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);

// State
enum Mode { AUTO, ON, OFF };
Mode lightMode = AUTO;
Mode fanMode = AUTO;
Mode alarmMode = OFF;

bool pirState = false;
bool authorized = true; // if false -> intrusion alarm
bool lightOn = false;
bool fanOn = false;
bool alarmOn = false;

unsigned long lastTelemetry = 0;
unsigned long lastMotion = 0;
const unsigned long motionHoldMs = 15000; // keep light on after last motion

// --- Helpers ---
void setRelay(int pin, bool on) {
  digitalWrite(pin, on ? HIGH : LOW);
}
void publishState(const char* topic, bool on) {
  mqtt.publish(topic, on ? "ON" : "OFF", true);
}
void publishMode(const char* topic, Mode m) {
  const char* s = (m == AUTO) ? "AUTO" : (m == ON ? "ON" : "OFF");
  mqtt.publish(topic, s, true);
}
void applyLogic() {
  // Read sensors
  int ldr = analogRead(LDR_PIN);
  bool isDark = (ldr < LDR_THRESHOLD);
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) { t = -100; h = -1; }

  // --- LIGHT decision ---
  bool lightDecision = false;
  if (lightMode == ON) lightDecision = true;
  else if (lightMode == OFF) lightDecision = false;
  else {
    // AUTO: light = PIR AND Dark (with hold-after-motion)
    bool motionActive = pirState || (millis() - lastMotion < motionHoldMs);
    lightDecision = motionActive && isDark;
  }

  // --- FAN decision ---
  bool fanDecision = false;
  if (fanMode == ON) fanDecision = true;
  else if (fanMode == OFF) fanDecision = false;
  else {
    // AUTO: Fan on if temp > threshold
    fanDecision = (t >= TEMP_THRESHOLD && t < 85.0);
  }

  // --- ALARM decision ---
  bool alarmDecision = false;
  if (alarmMode == ON) alarmDecision = true;
  else if (alarmMode == OFF) alarmDecision = false;
  else {
    // AUTO: Alarm on if PIR and NOT authorized
    alarmDecision = (pirState && !authorized);
  }

  // Apply outputs
  if (lightDecision != lightOn) {
    lightOn = lightDecision;
    setRelay(RELAY_LIGHT, lightOn);
    publishState(MQTT_LIGHT_STATE, lightOn);
  }
  if (fanDecision != fanOn) {
    fanOn = fanDecision;
    setRelay(RELAY_FAN, fanOn);
    publishState(MQTT_FAN_STATE, fanOn);
  }
  if (alarmDecision != alarmOn) {
    alarmOn = alarmDecision;
    digitalWrite(BUZZER, alarmOn ? HIGH : LOW);
    publishState(MQTT_ALARM_STATE, alarmOn);
  }

  // Telemetry periodically
  if (millis() - lastTelemetry > 5000) {
    lastTelemetry = millis();
    char buf[192];
    snprintf(buf, sizeof(buf),
      "{\"t\":%.1f,\"h\":%.1f,\"ldr\":%d,\"pir\":%d,\"light\":\"%s\",\"fan\":\"%s\",\"alarm\":\"%s\",\"lm\":\"%d\",\"fm\":\"%d\",\"am\":\"%d\"}",
      t, h, ldr, pirState ? 1 : 0,
      lightOn ? "ON" : "OFF",
      fanOn ? "ON" : "OFF",
      alarmOn ? "ON" : "OFF",
      (int)lightMode, (int)fanMode, (int)alarmMode);
    mqtt.publish(MQTT_TELEMETRY, buf, false);
  }
}

// --- Web UI ---
const char* PAGE = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Home</title>
<style>
body{font-family:system-ui,Arial;margin:20px} .card{border:1px solid #ddd;border-radius:12px;padding:16px;margin-bottom:12px}
h1{font-size:20px} button{padding:10px 14px;border-radius:10px;border:1px solid #aaa;cursor:pointer}
.grid{display:grid;gap:10px;grid-template-columns:repeat(auto-fit,minmax(160px,1fr))}
.badge{padding:6px 10px;border-radius:999px;border:1px solid #ccc}
</style></head><body>
<h1>ESP32 Home Automation</h1>
<div class="grid">
  <div class="card"><h3>Light</h3>
    <div id="ls" class="badge">state: ?</div><br/>
    <button onclick="cmd('light','ON')">ON</button>
    <button onclick="cmd('light','OFF')">OFF</button>
    <button onclick="cmd('light','AUTO')">AUTO</button>
  </div>
  <div class="card"><h3>Fan</h3>
    <div id="fs" class="badge">state: ?</div><br/>
    <button onclick="cmd('fan','ON')">ON</button>
    <button onclick="cmd('fan','OFF')">OFF</button>
    <button onclick="cmd('fan','AUTO')">AUTO</button>
  </div>
  <div class="card"><h3>Alarm</h3>
    <div id="as" class="badge">state: ?</div><br/>
    <button onclick="cmd('alarm','ON')">ON</button>
    <button onclick="cmd('alarm','OFF')">OFF</button>
    <button onclick="cmd('alarm','AUTO')">AUTO</button>
  </div>
</div>
<div class="card"><h3>Telemetry</h3><pre id="tele">loading...</pre></div>
<script>
async function cmd(dev, val){
  await fetch('/api/cmd?dev='+dev+'&val='+val);
  refresh();
}
async function refresh(){
  let r = await fetch('/api/state'); let j = await r.json();
  document.getElementById('ls').innerText = 'state: '+j.light.state+' | mode: '+j.light.mode;
  document.getElementById('fs').innerText = 'state: '+j.fan.state+' | mode: '+j.fan.mode;
  document.getElementById('as').innerText = 'state: '+j.alarm.state+' | mode: '+j.alarm.mode;
  document.getElementById('tele').innerText = JSON.stringify(j.telemetry,null,2);
}
setInterval(refresh, 3000); refresh();
</script>
</body></html>
)HTML";

String modeToStr(Mode m){ return m==AUTO?"AUTO":(m==ON?"ON":"OFF"); }

void handleRoot(){ server.send(200, "text/html", PAGE); }
void handleState(){
  int ldr = analogRead(LDR_PIN);
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  String json = "{";
  json += "\"light\":{\"state\":\""+ String(lightOn?"ON":"OFF") +"\",\"mode\":\""+modeToStr(lightMode)+"\"},";
  json += "\"fan\":{\"state\":\""+ String(fanOn?"ON":"OFF") +"\",\"mode\":\""+modeToStr(fanMode)+"\"},";
  json += "\"alarm\":{\"state\":\""+ String(alarmOn?"ON":"OFF") +"\",\"mode\":\""+modeToStr(alarmMode)+"\"},";
  json += "\"telemetry\":{\"t\":"+String(t,1)+",\"h\":"+String(h,1)+",\"ldr\":"+String(ldr)+",\"pir\":"+(pirState?String(1):String(0))+"}";
  json += "}";
  server.send(200, "application/json", json);
}
void handleCmd(){
  String dev = server.arg("dev");
  String val = server.arg("val");
  Mode m = AUTO;
  if (val=="ON") m = ON; else if (val=="OFF") m = OFF; else m = AUTO;
  if (dev=="light") { lightMode = m; publishMode(MQTT_LIGHT_MODE, lightMode); }
  if (dev=="fan")   { fanMode = m; publishMode(MQTT_FAN_MODE, fanMode); }
  if (dev=="alarm") { alarmMode = m; publishMode(MQTT_ALARM_MODE, alarmMode); }
  applyLogic();
  server.send(200, "text/plain", "OK");
}

// --- MQTT ---
void onMqttMessage(char* topic, byte* payload, unsigned int len){
  String msg; for(unsigned int i=0;i<len;i++) msg += (char)payload[i];
  auto setMode = [&](const String& dev, Mode &m){
    if (msg=="ON") m=ON; else if (msg=="OFF") m=OFF; else m=AUTO;
  };
  if (String(topic)==MQTT_LIGHT_CMD) { setMode("light", lightMode); publishMode(MQTT_LIGHT_MODE, lightMode); }
  if (String(topic)==MQTT_FAN_CMD)   { setMode("fan", fanMode); publishMode(MQTT_FAN_MODE, fanMode); }
  if (String(topic)==MQTT_ALARM_CMD) { setMode("alarm", alarmMode); publishMode(MQTT_ALARM_MODE, alarmMode); }
  applyLogic();
}

void connectWifi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to %s", WIFI_SSID);
  int tries=0;
  while (WiFi.status()!=WL_CONNECTED && tries<40){
    delay(500); Serial.print(".");
    tries++;
  }
  Serial.println();
  if (WiFi.status()==WL_CONNECTED){
    Serial.print("WiFi OK. IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi FAILED (continuing with BLE/local)");
  }
}

void connectMqtt(){
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  if (WiFi.status()!=WL_CONNECTED) return;
  String cid = "esp32-home-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  if (mqtt.connect(cid.c_str())){
    mqtt.subscribe(MQTT_LIGHT_CMD);
    mqtt.subscribe(MQTT_FAN_CMD);
    mqtt.subscribe(MQTT_ALARM_CMD);
    publishMode(MQTT_LIGHT_MODE, lightMode);
    publishMode(MQTT_FAN_MODE, fanMode);
    publishMode(MQTT_ALARM_MODE, alarmMode);
    Serial.println("MQTT connected");
  } else {
    Serial.println("MQTT connect failed");
  }
}

// --- BLE UART (optional minimal) ---
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEServer.h"

BLEServer *pServer = nullptr;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;

#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; }
  void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rx = pCharacteristic->getValue();
    String s = String(rx.c_str()); s.trim(); s.toUpperCase();
    if (s.indexOf("LIGHT ")==0){
      String v = s.substring(6); if (v=="ON") lightMode=ON; else if (v=="OFF") lightMode=OFF; else lightMode=AUTO;
    } else if (s.indexOf("FAN ")==0){
      String v = s.substring(4); if (v=="ON") fanMode=ON; else if (v=="OFF") fanMode=OFF; else fanMode=AUTO;
    } else if (s.indexOf("ALARM ")==0){
      String v = s.substring(6); if (v=="ON") alarmMode=ON; else if (v=="OFF") alarmMode=OFF; else alarmMode=AUTO;
    } else if (s.indexOf("TEMP ")==0){
      TEMP_THRESHOLD = s.substring(5).toFloat();
    } else if (s.indexOf("LDR ")==0){
      LDR_THRESHOLD = s.substring(4).toInt();
    }
    applyLogic();
  }
};

void setupBLE(){
  BLEDevice::init("ESP32-Home");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();
}

// --- ISR & buttons ---
volatile bool btnLightPressed=false, btnFanPressed=false;
void IRAM_ATTR onBtnLight(){ btnLightPressed=true; }
void IRAM_ATTR onBtnFan(){ btnFanPressed=true; }
void IRAM_ATTR onPir(){
  pirState = digitalRead(PIR_PIN)==HIGH;
  if (pirState) lastMotion = millis();
}

// --- Setup ---
void setup(){
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(RELAY_LIGHT, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BTN_LIGHT, INPUT_PULLUP);
  pinMode(BTN_FAN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), onPir, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BTN_LIGHT), onBtnLight, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_FAN), onBtnFan, FALLING);

  dht.begin();

  // Networking
  connectWifi();
  if (WiFi.status()==WL_CONNECTED){
    connectMqtt();
  }
  setupBLE(); // will remain active; can be disabled when MQTT is stable

  // Web
  server.on("/", handleRoot);
  server.on("/api/state", handleState);
  server.on("/api/cmd", handleCmd);
  server.begin();

  // Initial states
  setRelay(RELAY_LIGHT, false);
  setRelay(RELAY_FAN, false);
  digitalWrite(BUZZER, LOW);

  Serial.println("Setup complete.");
}

// --- Loop ---
void loop(){
  if (WiFi.status()==WL_CONNECTED){
    if (!mqtt.connected()) connectMqtt();
    mqtt.loop();
  }
  server.handleClient();

  // Handle buttons (toggle between ON and AUTO quickly)
  if (btnLightPressed){ btnLightPressed=false; lightMode = (lightMode==ON?AUTO:ON); publishMode(MQTT_LIGHT_MODE, lightMode); }
  if (btnFanPressed){ btnFanPressed=false; fanMode = (fanMode==ON?AUTO:ON); publishMode(MQTT_FAN_MODE, fanMode); }

  // Update PIR state (backup if interrupt missed)
  bool p = digitalRead(PIR_PIN)==HIGH;
  if (p && !pirState){ lastMotion = millis(); }
  pirState = p;

  applyLogic();

  // Light sleep when idle (no motion and not alarming)
  if (!pirState && !alarmOn){
    // Short light sleep to save power but maintain Wi‑Fi
    delay(10); // simple placeholder; deeper light sleep can be configured
  } else {
    delay(5);
  }
}
