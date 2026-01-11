# ESP32-S3 - Sensor de Temperatura DS18B20
## Guía de Configuración y Solución de Problemas

---

## 📋 Contenido
1. [Requisitos](#requisitos)
2. [Configuración en Arduino IDE](#configuración-en-arduino-ide)
3. [Problemas Comunes y Soluciones](#problemas-comunes-y-soluciones)
4. [Conexiones Hardware](#conexiones-hardware)

---

## 🔧 Requisitos

### Hardware
- **Placa:** ESP32-S3 DevKit-C (o compatible con chip ESP32-S3)
- **Sensor:** DS18B20 (1 o 2 sensores)
- **Resistencia:** 4.7kΩ pull-up para el bus OneWire
- **Cable:** USB-C (con datos, no solo carga)

### Software
- **Arduino IDE:** v1.8.19 o superior / v2.x
- **Librerías:** 
  - OneWire
  - DallasTemperature
  - WiFi (incluida en ESP32)
  - WebServer (incluida en ESP32)

---

## ⚙️ Configuración en Arduino IDE

### 1️⃣ Instalación del Driver USB (CRÍTICO)

#### **Para ESP32-S3 con chip USB nativo:**
El ESP32-S3 puede usar su chip USB nativo (no necesita CH340/CP2102).

**Verificación en Windows:**
1. Conectar el ESP32-S3 por USB
2. Abrir **Administrador de dispositivos** (Win + X → Administrador de dispositivos)
3. Buscar en **Puertos (COM y LPT)**
4. Debe aparecer como: `USB-SERIAL CH340 (COMx)` o `USB Serial Port (COMx)`

**Si NO aparece o tiene triángulo amarillo:**
1. Descargar driver CH340: https://sparks.gogo.co.nz/ch340.html
2. Ejecutar el instalador como Administrador
3. Reiniciar el PC
4. Verificar nuevamente en Administrador de dispositivos

**Verificación por PowerShell:**
```powershell
Get-PnpDevice | Where-Object {$_.FriendlyName -like "*USB*"} | Select-Object Status, Class, FriendlyName
```
Debe mostrar `Status: OK`

---

### 2️⃣ Configuración del Arduino IDE

#### **Instalar soporte para ESP32:**
1. Abrir Arduino IDE
2. Ir a: **Archivo → Preferencias**
3. En "Gestor de URLs Adicionales de Tarjetas" agregar:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Ir a: **Herramientas → Placa → Gestor de tarjetas**
5. Buscar "esp32" por **Espressif Systems**
6. Instalar versión **2.0.x** o superior

#### **Selección de la Placa (IMPORTANTE):**
```
Herramientas → Placa → esp32 → ESP32S3 Dev Module
```

**⚠️ CRÍTICO:** Si se selecciona mal la placa (ej: ESP32 Dev Module en lugar de ESP32S3), aparecerá el error:
```
A fatal error occurred: This chip is ESP32-S3 not ESP32. Wrong --chip argument?
```

---

### 3️⃣ Configuración de Parámetros de Carga

```
Herramientas → Configuración:
├── Placa: "ESP32S3 Dev Module"
├── Puerto: "COMx" (el que aparece en Administrador de dispositivos)
├── Upload Speed: "115200" (más estable que 921600)
├── USB CDC On Boot: "Enabled" (importante para Serial)
├── USB DFU On Boot: "Disabled"
├── Flash Mode: "QIO" o "DIO"
├── Flash Size: "8MB" (verificar según tu módulo)
├── Partition Scheme: "Default 4MB with spiffs"
└── PSRAM: "OPI PSRAM" (si tu módulo tiene PSRAM)
```

---

## 🚨 Problemas Comunes y Soluciones

### **Problema 1: "Cannot configure port, PermissionError(13)"**

**Causa:** El puerto COM está siendo utilizado por otro proceso.

**Solución:**
1. **CERRAR completamente el Serial Monitor** antes de cargar código
2. Verificar que no haya otros programas usando el puerto:
   - Cerrar PuTTY, Tera Term, otros IDEs
   - Buscar procesos `python.exe` en el Administrador de Tareas
3. Desconectar y reconectar el USB
4. Reiniciar Arduino IDE

---

### **Problema 2: "This chip is ESP32-S3 not ESP32"**

**Causa:** Placa mal seleccionada en Arduino IDE.

**Solución:**
```
Herramientas → Placa → esp32 → ESP32S3 Dev Module
```
**NO seleccionar:**
- ❌ ESP32 Dev Module
- ❌ ESP32 Wrover Module
- ❌ ESP32-S2

---

### **Problema 3: "Connecting..." sin respuesta**

**Causa:** El ESP32 no está entrando en modo bootloader.

**Solución manual:**
1. Mantén presionado el botón **BOOT** (o GPIO0)
2. Presiona y suelta el botón **RST** (Reset)
3. Suelta el botón **BOOT**
4. Presiona "Upload" inmediatamente

**Solución automática (si no funciona):**
- Reducir velocidad de carga: `Upload Speed: 115200`
- Cambiar cable USB (algunos solo cargan, no transmiten datos)
- Probar otro puerto USB del PC

---

### **Problema 4: Driver no reconocido en Windows**

**Síntomas:**
- Dispositivo no aparece en Puertos COM
- Triángulo amarillo en Administrador de dispositivos
- "Dispositivo USB no reconocido"

**Solución:**
1. **Desinstalar dispositivo:**
   - Administrador de dispositivos → Clic derecho en el dispositivo → Desinstalar
   - Marcar "Eliminar software de controlador"
2. **Desconectar ESP32**
3. **Instalar driver CH340:**
   - Descargar: https://sparks.gogo.co.nz/ch340.html
   - Ejecutar como Administrador
4. **Reiniciar PC**
5. **Conectar ESP32** nuevamente
6. Verificar en Administrador de dispositivos que aparezca sin errores

---

### **Problema 5: "esptool.py failed with exit code 2"**

**Causa:** Varios factores posibles.

**Soluciones:**
1. Verificar configuración de Flash:
   ```
   Flash Mode: QIO o DIO
   Flash Frequency: 80MHz
   Flash Size: Según tu módulo (generalmente 4MB u 8MB)
   ```
2. Limpiar compilación: **Sketch → Limpiar carpeta de salida**
3. Verificar que el código compile sin errores
4. Probar con un sketch básico (Blink) para descartar problemas de código

---

## 🔌 Conexiones Hardware

### **DS18B20 - ESP32-S3:**
```
DS18B20          ESP32-S3
────────────     ─────────
VCC     ────────→ 3.3V
GND     ────────→ GND
DATA    ────────→ GPIO4

        ┌─ 3.3V
        │
       [4.7kΩ]  ← Resistencia Pull-up
        │
        └─ GPIO4 (DATA)
```

**Notas:**
- La resistencia de 4.7kΩ es **obligatoria** entre DATA y 3.3V
- Puedes conectar múltiples DS18B20 al mismo pin (bus OneWire)
- Cable recomendado: máximo 3 metros sin repetidor

---

## 📝 Configuración WiFi y Red

### **En el código `.ino`:**
```cpp
// Configuración WiFi
const char* ssid = "TU_RED_WIFI";
const char* password = "TU_PASSWORD";

// IP estática
IPAddress local_IP(192, 168, 2, 111);  // Cambiar según tu red
IPAddress gateway(192, 168, 2, 1);      // IP del router
IPAddress subnet(255, 255, 255, 0);
```

### **Número de sensores:**
```cpp
#define NRO_SENSORES 1  // 1 o 2 según tu configuración
```

---

## ✅ Checklist Pre-Carga

Antes de cargar código, verificar:

- [ ] Driver CH340 instalado y funcionando
- [ ] Dispositivo aparece en Puertos COM
- [ ] Placa seleccionada: **ESP32S3 Dev Module**
- [ ] Puerto COM correcto seleccionado
- [ ] Upload Speed: **115200**
- [ ] Serial Monitor **CERRADO**
- [ ] Cable USB con datos (no solo carga)
- [ ] Código compila sin errores

---

## 🔍 Diagnóstico Rápido

### **Verificar driver (PowerShell):**
```powershell
Get-PnpDevice | Where-Object {$_.FriendlyName -like "*USB*"} | Select-Object Status, Class, FriendlyName
```

### **Ver puerto COM (PowerShell):**
```powershell
Get-WmiObject Win32_SerialPort | Select-Object DeviceID, Description
```

### **Verificar comunicación:**
1. Cargar sketch básico (Blink)
2. Abrir Serial Monitor (115200 baud)
3. Verificar que aparezcan mensajes

---

## 📌 Replicación en Otro PC

### **Pasos para configurar en un nuevo PC:**

1. **Instalar Arduino IDE**
   - Descargar de: https://www.arduino.cc/en/software

2. **Instalar driver CH340**
   - Descargar: https://sparks.gogo.co.nz/ch340.html
   - Ejecutar como Administrador
   - Reiniciar PC

3. **Configurar Arduino IDE**
   - Agregar URL de ESP32 en Preferencias
   - Instalar plataforma ESP32 (Espressif Systems)

4. **Instalar librerías**
   - Sketch → Incluir Librería → Administrar Bibliotecas
   - Instalar: OneWire, DallasTemperature

5. **Configurar parámetros**
   - Placa: ESP32S3 Dev Module
   - Puerto: Seleccionar COM correcto
   - Upload Speed: 115200
   - USB CDC On Boot: Enabled

6. **Verificar conexión**
   - Cargar ejemplo Blink
   - Si funciona, cargar código del sensor

---

## 📚 Referencias

- **Documentación ESP32:** https://docs.espressif.com/projects/esp-idf/
- **Arduino ESP32:** https://github.com/espressif/arduino-esp32
- **Librería DS18B20:** https://github.com/milesburton/Arduino-Temperature-Control-Library
- **Driver CH340:** https://sparks.gogo.co.nz/ch340.html

---

## 📧 Notas Finales

- **Guardar configuración:** Una vez que funcione, documentar el puerto COM usado
- **Backup:** Mantener copia del driver CH340 por si se necesita reinstalar
- **Testing:** Siempre probar con sketch simple antes de cargar código complejo

---

**Última actualización:** Enero 2026  
**Versión:** 1.0
