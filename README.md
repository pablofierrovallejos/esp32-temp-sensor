# ESP8266 / ESP32-S3 - Sensor de Temperatura DS18B20 (HTTP + MQTT)

Proyecto para leer 1 o 2 sensores DS18B20 en ESP8266 o ESP32-S3 con:
- API HTTP local
- Publicación MQTT
- Cache de lectura periódica (sin lecturas duplicadas por endpoint)

## Estructura

- `/tmp/workspace/pablofierrovallejos/esp32-temp-sensor/esp8266_ds18b20/esp8266_ds18b20.ino`
- `/tmp/workspace/pablofierrovallejos/esp32-temp-sensor/esp32_ds18b20/esp32_ds18b20.ino`

## Librerías requeridas

Instalar desde Arduino IDE:
1. **OneWire** (Paul Stoffregen)
2. **DallasTemperature** (Miles Burton)
3. **PubSubClient** (Nick O'Leary)

## Configuración segura (sin credenciales hardcodeadas)

Los sketches ya no traen SSID/password ni broker reales. Configura por build flags (`-D`) o editando macros antes de compilar:

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `MQTT_HOST`
- `MQTT_PORT` (default `1883`)
- `MQTT_USERNAME` / `MQTT_PASSWORD` (opcionales)
- `MQTT_USE_TLS` (`0` o `1`)
- `MQTT_BASE_PREFIX` (default `esp-temp`)
- `NRO_SENSORES` (`1` o `2`)
- `SAMPLE_INTERVAL_MS`

## Endpoints HTTP

- `/` interfaz web
- `/api/temperature` payload completo
- `/api/sensor1`
- `/api/sensor2` (si `NRO_SENSORES >= 2`)
- `/api/simple`
- `/api/health` diagnóstico de red, MQTT y sensores

## Contrato MQTT

Base topic por dispositivo:

`<MQTT_BASE_PREFIX>/<DEVICE_ID>/...`

Topics publicados:
- `availability` (`online`/`offline`, retained)
- `telemetry` (JSON completo)
- `sensor/1` (JSON sensor 1, retained)
- `sensor/2` (JSON sensor 2, retained si aplica)
- `health` (JSON de estado, retained)
- `status` (estado de configuración, retained)

Topic de comandos:
- `cmd/sampling_ms` -> permite ajustar intervalo de muestreo en runtime (1000-60000 ms)

## Características implementadas

- Muestreo único periódico no bloqueante por lógica de loop
- HTTP y MQTT consumen el mismo estado en memoria
- Reconexión WiFi/MQTT con backoff
- Cola offline corta para mensajes MQTT
- Publicación periódica + publicación por cambio de temperatura (delta)

## Nota sobre QoS

`PubSubClient` publica en QoS 0. Se usa `retained` para preservar últimos valores clave (availability/sensor/health/status).
