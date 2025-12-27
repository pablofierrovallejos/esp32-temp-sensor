# ESP8266 - Sensor de Temperatura DS18B20

Este proyecto lee la temperatura usando un sensor DS18B20 conectado a un ESP8266.

## Componentes necesarios

- ESP8266 (NodeMCU o similar)
- Sensor DS18B20
- Resistencia de 4.7kΩ (pull-up)
- Cables de conexión

## Conexiones

| DS18B20 | ESP8266 |
|---------|---------|
| VCC     | 3.3V    |
| GND     | GND     |
| DATA    | D4 (GPIO2) |

**Importante:** Conectar una resistencia de 4.7kΩ entre el pin DATA y VCC (3.3V).

## Librerías requeridas

Instalar las siguientes librerías desde el Administrador de Librerías de Arduino IDE:

1. **OneWire** por Paul Stoffregen
2. **DallasTemperature** por Miles Burton

### Cómo instalar las librerías:
1. Abrir Arduino IDE
2. Ir a `Sketch` → `Incluir Librería` → `Administrar Bibliotecas`
3. Buscar "OneWire" e instalar
4. Buscar "DallasTemperature" e instalar

## Configuración del ESP8266 en Arduino IDE

Si no tienes configurado el ESP8266 en Arduino IDE:

1. Ir a `Archivo` → `Preferencias`
2. En "Gestor de URLs Adicionales de Tarjetas", añadir:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Ir a `Herramientas` → `Placa` → `Gestor de tarjetas`
4. Buscar "esp8266" e instalar "esp8266 by ESP8266 Community"
5. Seleccionar tu placa en `Herramientas` → `Placa` → `ESP8266 Boards` → `NodeMCU 1.0 (ESP-12E Module)` (o tu modelo específico)

## Uso

1. Cargar el sketch `esp8266_ds18b20.ino` en tu ESP8266
2. Abrir el Monitor Serial a 115200 baudios
3. Verás las lecturas de temperatura cada 2 segundos

## Salida esperada

```
ESP8266 - Sensor DS18B20
Iniciando sensores...
Sensores encontrados: 1
Solicitando temperatura... OK
Temperatura: 23.50 °C
Temperatura: 74.30 °F
-----------------------------------
```

## Solución de problemas

Si ves "Sensores encontrados: 0":
- Verifica todas las conexiones
- Asegúrate de tener la resistencia pull-up de 4.7kΩ conectada
- Comprueba que el sensor no esté dañado
- Verifica que estés usando el pin correcto (D4 = GPIO2)

Si ves "Error: No se pudo leer la temperatura":
- El sensor podría estar desconectado
- Verifica la alimentación (3.3V)
- Intenta con otro sensor si es posible
