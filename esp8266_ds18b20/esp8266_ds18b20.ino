/*
 * ESP8266 - Sensor DS18B20 con HTTP + MQTT
 * Lectura periódica única y publicación robusta
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include <math.h>
#if MQTT_USE_TLS
#include <WiFiClientSecure.h>
#endif

// ====== Configuración (sobrescribir con -D en build flags) ======
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

#ifndef WIFI_USE_STATIC_IP
#define WIFI_USE_STATIC_IP 0
#endif

#ifndef WIFI_LOCAL_IP
#define WIFI_LOCAL_IP IPAddress(192, 168, 2, 110)
#endif
#ifndef WIFI_GATEWAY
#define WIFI_GATEWAY IPAddress(192, 168, 2, 1)
#endif
#ifndef WIFI_SUBNET
#define WIFI_SUBNET IPAddress(255, 255, 255, 0)
#endif
#ifndef WIFI_PRIMARY_DNS
#define WIFI_PRIMARY_DNS IPAddress(8, 8, 8, 8)
#endif
#ifndef WIFI_SECONDARY_DNS
#define WIFI_SECONDARY_DNS IPAddress(8, 8, 4, 4)
#endif

#ifndef MQTT_HOST
#define MQTT_HOST ""
#endif
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif
#ifndef MQTT_USERNAME
#define MQTT_USERNAME ""
#endif
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif
#ifndef MQTT_USE_TLS
#define MQTT_USE_TLS 0
#endif
#ifndef MQTT_BASE_PREFIX
#define MQTT_BASE_PREFIX "esp-temp"
#endif

#ifndef ONE_WIRE_BUS
#define ONE_WIRE_BUS 4
#endif

#ifndef NRO_SENSORES
#define NRO_SENSORES 1
#endif

#ifndef SAMPLE_INTERVAL_MS
#define SAMPLE_INTERVAL_MS 2000UL
#endif
#ifndef MQTT_PUBLISH_INTERVAL_MS
#define MQTT_PUBLISH_INTERVAL_MS 10000UL
#endif
#ifndef HEALTH_PUBLISH_INTERVAL_MS
#define HEALTH_PUBLISH_INTERVAL_MS 30000UL
#endif
#ifndef WIFI_RECONNECT_INTERVAL_MS
#define WIFI_RECONNECT_INTERVAL_MS 5000UL
#endif
#ifndef MQTT_RECONNECT_INTERVAL_MS
#define MQTT_RECONNECT_INTERVAL_MS 5000UL
#endif
#ifndef TEMP_DELTA_THRESHOLD
#define TEMP_DELTA_THRESHOLD 0.3f
#endif

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* mqttHost = MQTT_HOST;
const uint16_t mqttPort = MQTT_PORT;
const char* mqttUsername = MQTT_USERNAME;
const char* mqttPassword = MQTT_PASSWORD;

IPAddress local_IP = WIFI_LOCAL_IP;
IPAddress gateway = WIFI_GATEWAY;
IPAddress subnet = WIFI_SUBNET;
IPAddress primaryDNS = WIFI_PRIMARY_DNS;
IPAddress secondaryDNS = WIFI_SECONDARY_DNS;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
ESP8266WebServer server(80);

#if MQTT_USE_TLS
WiFiClientSecure transportClient;
#else
WiFiClient transportClient;
#endif
PubSubClient mqttClient(transportClient);

struct SensorCache {
  float sensor1C;
  float sensor2C;
  bool sensor1Error;
  bool sensor2Error;
  int detected;
  unsigned long sampleMillis;
};

struct PendingMessage {
  String topic;
  String payload;
  bool retained;
};

const int OFFLINE_QUEUE_SIZE = 8;
PendingMessage pendingQueue[OFFLINE_QUEUE_SIZE];
int pendingStart = 0;
int pendingCount = 0;

SensorCache current = {0.0f, 0.0f, true, true, 0, 0};

String deviceId;
String mqttBase;
String topicAvailability;
String topicTelemetry;
String topicSensor1;
String topicSensor2;
String topicHealth;
String topicStatus;
String topicCmdSampling;

unsigned long lastSampleAt = 0;
unsigned long lastPublishAt = 0;
unsigned long lastHealthPublishAt = 0;
unsigned long lastWifiAttemptAt = 0;
unsigned long lastMqttAttemptAt = 0;
unsigned long mqttReconnects = 0;
unsigned long wifiReconnects = 0;
unsigned long sensorReadErrors = 0;
unsigned long sampleIntervalMs = SAMPLE_INTERVAL_MS;
float lastPublishedTemp1 = NAN;
float lastPublishedTemp2 = NAN;

void logInfo(const String& msg) { Serial.println("[INFO] " + msg); }
void logWarn(const String& msg) { Serial.println("[WARN] " + msg); }
void logError(const String& msg) { Serial.println("[ERROR] " + msg); }

void setCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

bool hasMqttConfig() {
  return String(mqttHost).length() > 0;
}

bool enqueueMessage(const String& topic, const String& payload, bool retained) {
  if (pendingCount >= OFFLINE_QUEUE_SIZE) {
    pendingStart = (pendingStart + 1) % OFFLINE_QUEUE_SIZE;
    pendingCount--;
  }
  int idx = (pendingStart + pendingCount) % OFFLINE_QUEUE_SIZE;
  pendingQueue[idx].topic = topic;
  pendingQueue[idx].payload = payload;
  pendingQueue[idx].retained = retained;
  pendingCount++;
  return true;
}

bool publishRaw(const String& topic, const String& payload, bool retained) {
  return mqttClient.publish(topic.c_str(), payload.c_str(), retained);
}

bool publishOrQueue(const String& topic, const String& payload, bool retained) {
  if (mqttClient.connected()) {
    if (publishRaw(topic, payload, retained)) return true;
  }
  return enqueueMessage(topic, payload, retained);
}

void flushQueue() {
  while (pendingCount > 0 && mqttClient.connected()) {
    PendingMessage& m = pendingQueue[pendingStart];
    if (!publishRaw(m.topic, m.payload, m.retained)) break;
    pendingStart = (pendingStart + 1) % OFFLINE_QUEUE_SIZE;
    pendingCount--;
  }
}

void acquireTemperatureSample() {
  sensors.requestTemperatures();

  current.detected = sensors.getDeviceCount();
  current.sensor1C = sensors.getTempCByIndex(0);
  current.sensor1Error = !(current.sensor1C != DEVICE_DISCONNECTED_C && current.sensor1C < 85.0f && current.sensor1C > -55.0f);

  if (NRO_SENSORES >= 2 && current.detected >= 2) {
    current.sensor2C = sensors.getTempCByIndex(1);
    current.sensor2Error = !(current.sensor2C != DEVICE_DISCONNECTED_C && current.sensor2C < 85.0f && current.sensor2C > -55.0f);
  } else {
    current.sensor2C = 0.0f;
    current.sensor2Error = true;
  }

  if (current.sensor1Error || (NRO_SENSORES >= 2 && current.sensor2Error)) sensorReadErrors++;
  current.sampleMillis = millis();
}

String telemetryJson() {
  String json = "{";
  json += "\"deviceId\":\"" + deviceId + "\",";
  json += "\"timestamp\":" + String(current.sampleMillis) + ",";
  json += "\"uptimeMs\":" + String(millis()) + ",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"sensor1\":{";
  json += "\"error\":" + String(current.sensor1Error ? "true" : "false") + ",";
  json += "\"temperatureC\":" + String(current.sensor1Error ? 0 : current.sensor1C, 2);
  json += "}";
  if (NRO_SENSORES >= 2) {
    json += ",\"sensor2\":{";
    json += "\"error\":" + String(current.sensor2Error ? "true" : "false") + ",";
    json += "\"temperatureC\":" + String(current.sensor2Error ? 0 : current.sensor2C, 2);
    json += "}";
  }
  json += ",\"meta\":{";
  json += "\"configuredSensors\":" + String(NRO_SENSORES) + ",";
  json += "\"detectedSensors\":" + String(current.detected);
  json += "}";
  json += "}";
  return json;
}

String sensorJson(int idx) {
  bool err = idx == 1 ? current.sensor1Error : current.sensor2Error;
  float temp = idx == 1 ? current.sensor1C : current.sensor2C;
  String json = "{";
  json += "\"deviceId\":\"" + deviceId + "\",";
  json += "\"sensor\":" + String(idx) + ",";
  json += "\"timestamp\":" + String(current.sampleMillis) + ",";
  json += "\"unit\":\"C\",";
  json += "\"error\":" + String(err ? "true" : "false") + ",";
  json += "\"temperature\":" + String(err ? 0 : temp, 2);
  json += "}";
  return json;
}

String healthJson() {
  String json = "{";
  json += "\"deviceId\":\"" + deviceId + "\",";
  json += "\"uptimeMs\":" + String(millis()) + ",";
  json += "\"wifiConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"mqttConnected\":" + String(mqttClient.connected() ? "true" : "false") + ",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"wifiReconnects\":" + String(wifiReconnects) + ",";
  json += "\"mqttReconnects\":" + String(mqttReconnects) + ",";
  json += "\"sensorReadErrors\":" + String(sensorReadErrors) + ",";
  json += "\"queueDepth\":" + String(pendingCount) + ",";
  json += "\"sampleAgeMs\":" + String(millis() - current.sampleMillis);
  json += "}";
  return json;
}

void publishAvailability(const char* state) {
  if (!hasMqttConfig()) return;
  publishOrQueue(topicAvailability, String(state), true);
}

void publishTelemetryIfNeeded() {
  if (!hasMqttConfig()) return;
  bool due = (millis() - lastPublishAt) >= MQTT_PUBLISH_INTERVAL_MS;
  bool changed = (!current.sensor1Error && (isnan(lastPublishedTemp1) || fabs(current.sensor1C - lastPublishedTemp1) >= TEMP_DELTA_THRESHOLD));
  if (NRO_SENSORES >= 2 && !current.sensor2Error) {
    changed = changed || (isnan(lastPublishedTemp2) || fabs(current.sensor2C - lastPublishedTemp2) >= TEMP_DELTA_THRESHOLD);
  }

  if (!due && !changed) return;

  publishOrQueue(topicTelemetry, telemetryJson(), false);
  publishOrQueue(topicSensor1, sensorJson(1), true);
  if (NRO_SENSORES >= 2) publishOrQueue(topicSensor2, sensorJson(2), true);

  if (!current.sensor1Error) lastPublishedTemp1 = current.sensor1C;
  if (NRO_SENSORES >= 2 && !current.sensor2Error) lastPublishedTemp2 = current.sensor2C;
  lastPublishAt = millis();
}

void publishHealthIfNeeded() {
  if (!hasMqttConfig()) return;
  if ((millis() - lastHealthPublishAt) < HEALTH_PUBLISH_INTERVAL_MS) return;
  publishOrQueue(topicHealth, healthJson(), true);
  lastHealthPublishAt = millis();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String incomingTopic(topic);
  String body;
  for (unsigned int i = 0; i < length; i++) body += (char)payload[i];
  body.trim();

  if (incomingTopic == topicCmdSampling) {
    unsigned long requested = body.toInt();
    if (requested >= 1000 && requested <= 60000) {
      sampleIntervalMs = requested;
      publishOrQueue(topicStatus, String("{\"sampleIntervalMs\":") + String(sampleIntervalMs) + "}", true);
      logInfo("Nuevo sampleIntervalMs=" + String(sampleIntervalMs));
    }
  }
}

void connectWiFiNonBlocking() {
  if (String(ssid).length() == 0) return;
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastWifiAttemptAt < WIFI_RECONNECT_INTERVAL_MS) return;
  lastWifiAttemptAt = millis();
  wifiReconnects++;

  WiFi.disconnect();
#if WIFI_USE_STATIC_IP
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
#endif
  WiFi.begin(ssid, password);
  logWarn("Intentando reconexión WiFi...");
}

void connectMqttNonBlocking() {
  if (!hasMqttConfig()) return;
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) return;
  if (millis() - lastMqttAttemptAt < MQTT_RECONNECT_INTERVAL_MS) return;
  lastMqttAttemptAt = millis();

  String willTopic = topicAvailability;
  String clientId = "esp8266-" + deviceId;

  bool connected;
  if (String(mqttUsername).length() > 0) {
    connected = mqttClient.connect(clientId.c_str(), mqttUsername, mqttPassword, willTopic.c_str(), 1, true, "offline");
  } else {
    connected = mqttClient.connect(clientId.c_str(), willTopic.c_str(), 1, true, "offline");
  }

  if (connected) {
    mqttReconnects++;
    mqttClient.subscribe(topicCmdSampling.c_str());
    publishAvailability("online");
    publishOrQueue(topicStatus, String("{\"sampleIntervalMs\":") + String(sampleIntervalMs) + "}", true);
    flushQueue();
    logInfo("MQTT conectado");
  } else {
    logWarn("MQTT desconectado rc=" + String(mqttClient.state()));
  }
}

void handleRoot() {
  setCors();
  String html = "<!DOCTYPE html><html lang='es'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP8266 Temp Monitor</title><style>body{font-family:Arial;background:#f2f5ff;margin:0;padding:20px}.card{max-width:700px;margin:auto;background:#fff;border-radius:12px;padding:20px;box-shadow:0 8px 24px rgba(0,0,0,.12)}.temp{font-size:48px;color:#2d4ecf}.muted{color:#666}</style></head><body>";
  html += "<div class='card'><h1>🌡️ ESP8266 DS18B20</h1>";
  html += "<p class='temp'>" + String(current.sensor1Error ? 0 : current.sensor1C, 1) + " °C</p>";
  if (NRO_SENSORES >= 2) html += "<p class='temp'>S2: " + String(current.sensor2Error ? 0 : current.sensor2C, 1) + " °C</p>";
  html += "<p class='muted'>IP: " + WiFi.localIP().toString() + " | MQTT: " + String(mqttClient.connected() ? "Conectado" : "Desconectado") + "</p>";
  html += "<p class='muted'>Dispositivo: " + deviceId + "</p></div></body></html>";
  server.send(200, "text/html", html);
}

void handleAPI() { setCors(); server.send(200, "application/json", telemetryJson()); }
void handleSensor1() { setCors(); server.send(200, "application/json", sensorJson(1)); }

void handleSensor2() {
  setCors();
  if (NRO_SENSORES < 2) {
    server.send(400, "application/json", "{\"error\":true,\"message\":\"Sensor 2 no configurado\"}");
    return;
  }
  server.send(200, "application/json", sensorJson(2));
}

void handleSimple() {
  setCors();
  String json = "{\"temp1\":" + String(current.sensor1Error ? 0 : current.sensor1C, 2);
  if (NRO_SENSORES >= 2) json += ",\"temp2\":" + String(current.sensor2Error ? 0 : current.sensor2C, 2);
  json += "}";
  server.send(200, "application/json", json);
}

void handleHealth() { setCors(); server.send(200, "application/json", healthJson()); }
void handleNotFound() { setCors(); server.send(404, "text/plain", "404: Página no encontrada"); }

String buildDeviceId() {
  char buff[9];
  snprintf(buff, sizeof(buff), "%08X", ESP.getChipId());
  return String(buff);
}

void setupTopics() {
  mqttBase = String(MQTT_BASE_PREFIX) + "/" + deviceId;
  topicAvailability = mqttBase + "/availability";
  topicTelemetry = mqttBase + "/telemetry";
  topicSensor1 = mqttBase + "/sensor/1";
  topicSensor2 = mqttBase + "/sensor/2";
  topicHealth = mqttBase + "/health";
  topicStatus = mqttBase + "/status";
  topicCmdSampling = mqttBase + "/cmd/sampling_ms";
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\nESP8266 DS18B20 HTTP+MQTT");

  deviceId = buildDeviceId();
  setupTopics();

  sensors.begin();
  acquireTemperatureSample();

  WiFi.mode(WIFI_STA);
#if WIFI_USE_STATIC_IP
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
#endif
  if (String(ssid).length() > 0) WiFi.begin(ssid, password);

#if MQTT_USE_TLS
  transportClient.setInsecure();
#endif
  mqttClient.setServer(mqttHost, mqttPort);
  mqttClient.setCallback(mqttCallback);

  server.on("/", handleRoot);
  server.on("/api/temperature", handleAPI);
  server.on("/api/sensor1", handleSensor1);
  server.on("/api/sensor2", handleSensor2);
  server.on("/api/simple", handleSimple);
  server.on("/api/health", handleHealth);
  server.onNotFound(handleNotFound);
  server.begin();

  logInfo("HTTP listo. DeviceId=" + deviceId);
  logInfo("Configura WIFI_SSID/WIFI_PASSWORD y MQTT_HOST via build flags.");
}

void loop() {
  unsigned long now = millis();

  connectWiFiNonBlocking();
  connectMqttNonBlocking();

  if (mqttClient.connected()) {
    mqttClient.loop();
    flushQueue();
  }

  if (now - lastSampleAt >= sampleIntervalMs) {
    acquireTemperatureSample();
    lastSampleAt = now;
  }

  publishTelemetryIfNeeded();
  publishHealthIfNeeded();

  server.handleClient();
}
