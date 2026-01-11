/*
 * ESP32-S3 - Sensor de Temperatura DS18B20 con Web Server
 * Dos sensores en el mismo pin (bus OneWire)
 * 
 * Librerías necesarias:
 * - OneWire
 * - DallasTemperature
 * - WiFi (incluida en el core de ESP32)
 * - WebServer (incluida en el core de ESP32)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Configuración WiFi
const char* ssid = "belkin.3f0";
const char* password = "6e4496f6";

// Configuración IP estática
IPAddress local_IP(192, 168, 2, 111# Verificar si el dispositivo es detectado
Get-PnpDevice | Where-Object {$_.FriendlyName -like "*USB*"} | Select-Object Status, Class, FriendlyName);      // IP fija deseada
IPAddress gateway(192, 168, 2, 1);         // Gateway (router)
IPAddress subnet(255, 255, 255, 0);        // Máscara de subred
IPAddress primaryDNS(8, 8, 8, 8);          // DNS primario (Google)
IPAddress secondaryDNS(8, 8, 4, 4);        // DNS secundario (Google)

// Pin donde están conectados los sensores DS18B20 (ambos en el mismo bus)
#define ONE_WIRE_BUS 4  // GPIO4 en ESP32-S3

// *** CONFIGURACIÓN DE NÚMERO DE SENSORES ***
#define NRO_SENSORES 1  // Cambia este valor según el número de sensores que uses (1 o 2)

// Configurar OneWire para los sensores
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Crear servidor web en el puerto 80
WebServer server(80);

// Variables globales para almacenar temperatura
float temperature1C = 0.0;
float temperature2C = 0.0;
unsigned long lastUpdate = 0;
bool sensor1Error = false;
bool sensor2Error = false;
int numSensors = 0;

// Función para leer la temperatura
void updateTemperature() {
  sensors.requestTemperatures();
  numSensors = sensors.getDeviceCount();
  
  // Leer sensor 1
  temperature1C = sensors.getTempCByIndex(0);
  if (temperature1C != DEVICE_DISCONNECTED_C && temperature1C < 85.0 && temperature1C > -55.0) {
    sensor1Error = false;
  } else {
    sensor1Error = true;
  }
  
  // Leer sensor 2 solo si NRO_SENSORES >= 2
  if (NRO_SENSORES >= 2) {
    if (numSensors >= 2) {
      temperature2C = sensors.getTempCByIndex(1);
      if (temperature2C != DEVICE_DISCONNECTED_C && temperature2C < 85.0 && temperature2C > -55.0) {
        sensor2Error = false;
      } else {
        sensor2Error = true;
      }
    } else {
      sensor2Error = true;
    }
  } else {
    // Si NRO_SENSORES = 1, marcar sensor2 como no usado
    sensor2Error = true;
    temperature2C = 0.0;
  }
  
  lastUpdate = millis();
}

// Página web principal con diseño moderno
void handleRoot() {
  String html = "<!DOCTYPE html><html lang='es'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Sensor de Temperatura ESP32-S3</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); margin: 0; padding: 20px; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; border-radius: 20px; padding: 30px; box-shadow: 0 10px 40px rgba(0,0,0,0.3); }";
  html += "h1 { color: #667eea; text-align: center; margin-bottom: 30px; font-size: 28px; }";
  html += ".sensor-card { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; border-radius: 15px; text-align: center; margin-bottom: 20px; }";
  html += ".temperature { font-size: 60px; font-weight: bold; margin: 20px 0; }";
  html += ".unit { font-size: 30px; }";
  html += ".info { background: #f5f5f5; padding: 15px; border-radius: 10px; margin-top: 20px; }";
  html += ".info-row { display: flex; justify-content: space-between; padding: 10px 0; border-bottom: 1px solid #ddd; }";
  html += ".info-row:last-child { border-bottom: none; }";
  html += ".label { font-weight: bold; color: #555; }";
  html += ".value { color: #667eea; }";
  html += ".error { background: #ff4444; color: white; padding: 15px; border-radius: 10px; text-align: center; }";
  html += ".refresh-btn { background: #667eea; color: white; border: none; padding: 15px 40px; font-size: 16px; border-radius: 10px; cursor: pointer; margin-top: 20px; width: 100%; }";
  html += ".refresh-btn:hover { background: #764ba2; }";
  html += "@media (max-width: 600px) { .temperature { font-size: 48px; } }";
  html += "</style>";
  html += "<script>";
  html += "function refreshData() {";
  html += "  fetch('/api/temperature').then(r => r.json()).then(data => {";
  html += "    if(data.error) { document.getElementById('sensor-data').innerHTML = '<div class=\"error\">' + data.message + '</div>'; }";
  html += "    else { location.reload(); }";
  html += "  });";
  html += "}";
  html += "setInterval(refreshData, 5000);";  // Auto-refresh cada 5 segundos
  html += "</script>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>🌡️ Sensor de Temperatura</h1>";
  
  html += "<div id='sensor-data'>";
  
  // Sensor 1
  if (sensor1Error) {
    html += "<div class='error'>";
    html += "<h2>⚠️ Error Sensor 1</h2>";
    html += "<p>No se puede leer la temperatura</p>";
    html += "</div>";
  } else {
    html += "<div class='sensor-card'>";
    html += "<div>Sensor 1 (GPIO4)</div>";
    html += "<div class='temperature'>" + String(temperature1C, 1) + "<span class='unit'>°C</span></div>";
    html += "</div>";
  }
  
  // Sensor 2 - Solo mostrar si NRO_SENSORES >= 2
  if (NRO_SENSORES >= 2) {
    if (!sensor2Error) {
      html += "<div class='sensor-card' style='margin-top: 15px;'>";
      html += "<div>Sensor 2 (GPIO4)</div>";
      html += "<div class='temperature'>" + String(temperature2C, 1) + "<span class='unit'>°C</span></div>";
      html += "</div>";
    } else {
      html += "<div class='error' style='margin-top: 15px;'>";
      html += "<h2>⚠️ Error Sensor 2</h2>";
      html += "<p>Conecta ambos sensores DATA al pin GPIO4</p>";
      html += "<p>Cada uno con su resistencia 4.7kΩ</p>";
      html += "<p>Revisa las conexiones</p>";
      html += "</div>";
    }
  }
  
  html += "</div>";
  
  html += "<div class='info'>";
  html += "<div class='info-row'><span class='label'>Estado WiFi:</span><span class='value'>Conectado</span></div>";
  html += "<div class='info-row'><span class='label'>Red:</span><span class='value'>" + String(ssid) + "</span></div>";
  html += "<div class='info-row'><span class='label'>IP:</span><span class='value'>" + WiFi.localIP().toString() + "</span></div>";
  html += "<div class='info-row'><span class='label'>Chip:</span><span class='value'>ESP32-S3</span></div>";
  html += "<div class='info-row'><span class='label'>Sensores configurados:</span><span class='value'>" + String(NRO_SENSORES) + "</span></div>";
  html += "<div class='info-row'><span class='label'>Sensores detectados:</span><span class='value'>" + String(numSensors) + "</span></div>";
  html += "<div class='info-row'><span class='label'>Última lectura:</span><span class='value'>" + String((millis() - lastUpdate) / 1000) + "s</span></div>";
  html += "</div>";
  
  html += "<button class='refresh-btn' onclick='refreshData()'>🔄 Actualizar</button>";
  html += "</div>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// API endpoint para obtener datos JSON
void handleAPI() {
  updateTemperature();
  
  String json = "{";
  json += "\"nroSensoresConfig\": " + String(NRO_SENSORES) + ",";
  json += "\"numSensorsDetected\": " + String(numSensors) + ",";
  json += "\"timestamp\": " + String(millis()) + ",";
  
  // Sensor 1
  json += "\"sensor1\": {";
  if (sensor1Error) {
    json += "\"error\": true,";
    json += "\"temperature\": null";
  } else {
    json += "\"error\": false,";
    json += "\"temperature\": " + String(temperature1C, 2);
  }
  json += "}";
  
  // Sensor 2 - Solo incluir si NRO_SENSORES >= 2
  if (NRO_SENSORES >= 2) {
    json += ",\"sensor2\": {";
    if (sensor2Error) {
      json += "\"error\": true,";
      json += "\"temperature\": null";
    } else {
      json += "\"error\": false,";
      json += "\"temperature\": " + String(temperature2C, 2);
    }
    json += "}";
  }
  
  json += "}";
  
  server.send(200, "application/json", json);
}

// API endpoint para sensor 1
void handleSensor1() {
  updateTemperature();
  
  String json = "{";
  if (sensor1Error) {
    json += "\"error\": true,";
    json += "\"temperature\": null";
  } else {
    json += "\"error\": false,";
    json += "\"temperature\": " + String(temperature1C, 2);
  }
  json += ",\"timestamp\": " + String(millis());
  json += "}";
  
  server.send(200, "application/json", json);
}

// API endpoint para sensor 2
void handleSensor2() {
  // Solo permitir acceso si NRO_SENSORES >= 2
  if (NRO_SENSORES < 2) {
    String json = "{";
    json += "\"error\": true,";
    json += "\"message\": \"Sensor 2 no está configurado (NRO_SENSORES=1)\",";
    json += "\"temperature\": null";
    json += "}";
    server.send(400, "application/json", json);
    return;
  }
  
  updateTemperature();
  
  String json = "{";
  if (sensor2Error || numSensors < 2) {
    json += "\"error\": true,";
    json += "\"temperature\": null";
  } else {
    json += "\"error\": false,";
    json += "\"temperature\": " + String(temperature2C, 2);
  }
  json += ",\"timestamp\": " + String(millis());
  json += "}";
  
  server.send(200, "application/json", json);
}

// API endpoint simple - solo temperaturas
void handleSimple() {
  updateTemperature();
  
  String json = "{";
  json += "\"temp1\": " + String(sensor1Error ? 0 : temperature1C, 2);
  
  // Solo incluir temp2 si NRO_SENSORES >= 2
  if (NRO_SENSORES >= 2) {
    json += ",\"temp2\": " + String(sensor2Error ? 0 : temperature2C, 2);
  }
  
  json += "}";
  
  server.send(200, "application/json", json);
}

// Página 404
void handleNotFound() {
  server.send(404, "text/plain", "404: Página no encontrada");
}

void setup() {
  // Inicializar comunicación serial
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\nESP32-S3 - Sensor DS18B20 con Web Server");
  Serial.println("========================================");
  
  // Inicializar los sensores DS18B20
  Serial.println("Iniciando sensores...");
  sensors.begin();
  
  Serial.println("Configuración:");
  Serial.print("- Número de sensores configurado: ");
  Serial.println(NRO_SENSORES);
  if (NRO_SENSORES == 1) {
    Serial.println("- 1 sensor conectado a GPIO4");
    Serial.println("- Resistencia pull-up 4.7kΩ entre DATA y VCC");
  } else {
    Serial.println("- Sensores conectados a GPIO4");
    Serial.println("- Cada sensor con su resistencia pull-up 4.7kΩ");
  }
  Serial.print("- Sensores detectados: ");
  Serial.println(sensors.getDeviceCount());
  
  // Conectar a WiFi
  Serial.println("\nConectando a WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  
  // Configurar IP estática antes de conectar
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Error al configurar IP estática");
  }
  
  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  
  // Esperar conexión
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("¡WiFi conectado exitosamente!");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Accede al servidor web en: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Error: No se pudo conectar a WiFi");
    Serial.println("Verifica el SSID y la contraseña");
  }
  
  // Configurar rutas del servidor web
  server.on("/", handleRoot);
  server.on("/api/temperature", handleAPI);
  server.on("/api/sensor1", handleSensor1);
  server.on("/api/sensor2", handleSensor2);
  server.on("/api/simple", handleSimple);
  server.onNotFound(handleNotFound);
  
  // Iniciar servidor
  server.begin();
  Serial.println("Servidor web iniciado");
  Serial.println("\nEndpoints API disponibles:");
  Serial.println("- http://" + WiFi.localIP().toString() + " (interfaz web)");
  Serial.println("- http://" + WiFi.localIP().toString() + "/api/temperature (datos completos)");
  Serial.println("- http://" + WiFi.localIP().toString() + "/api/sensor1 (solo sensor 1)");
  if (NRO_SENSORES >= 2) {
    Serial.println("- http://" + WiFi.localIP().toString() + "/api/sensor2 (solo sensor 2)");
  }
  Serial.println("- http://" + WiFi.localIP().toString() + "/api/simple (formato simple)");
  Serial.println("========================================\n");
  
  // Primera lectura de temperatura
  updateTemperature();
}

void loop() {
  // Manejar peticiones del servidor web
  server.handleClient();
  
  // Actualizar temperatura cada 2 segundos
  if (millis() - lastUpdate > 2000) {
    updateTemperature();
    
    // Mostrar en Serial
    Serial.print("Sensores activos: ");
    Serial.println(numSensors);
    
    if (!sensor1Error) {
      Serial.print("Sensor 1 (GPIO4): ");
      Serial.print(temperature1C, 1);
      Serial.println(" °C");
    } else {
      Serial.println("Sensor 1 (GPIO4): Error de lectura");
    }
    
    // Solo mostrar sensor 2 si está configurado
    if (NRO_SENSORES >= 2) {
      if (!sensor2Error) {
        Serial.print("Sensor 2 (GPIO4): ");
        Serial.print(temperature2C, 1);
        Serial.println(" °C");
      } else {
        Serial.println("Sensor 2 (GPIO4): No detectado");
      }
    }
    
    Serial.println("---");
  }
}
