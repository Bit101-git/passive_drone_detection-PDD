# Sistema Pasivo de Detección de Drones FPV

Sistema de **monitoreo pasivo de radiofrecuencia basado en ESP32**, diseñado para detectar actividad RF asociada a drones FPV.

El proyecto combina la **detección de video analógico en 5.8 GHz** mediante un receptor RX5808 con el **monitoreo de actividad RF en 2.4 GHz** mediante un NRF24L01. El sistema analiza la intensidad de señal, estabilidad, persistencia y actividad en frecuencia para identificar posibles transmisiones de video FPV y patrones de radiofrecuencia compatibles con FHSS.

> **Nota:** El detector de 2.4 GHz identifica actividad RF y patrones de salto de frecuencia compatibles con FHSS. **No identifica de manera inequívoca ni criptográfica el protocolo ExpressLRS (ELRS).**

---

## ✨ Características

* 📡 Monitoreo pasivo de radiofrecuencia
* 📺 Detección de video FPV analógico
* 📶 Monitoreo de actividad RF en 2.4 GHz
* 🔍 Escaneo automático de **72 frecuencias FPV**
* 📊 Análisis de señal RSSI
* 📈 Cálculo de media y desviación estándar
* 🎯 Umbrales dinámicos de detección
* 🔄 Calibración automática del nivel base de ruido RSSI
* 🧠 Puntaje de confianza de señal de **0 a 100**
* ⏱️ Análisis de persistencia de detección
* 🔒 Histéresis para bloqueo y pérdida de señal
* 🚨 Detección de cambios de canal y banda
* 📡 Detección de actividad compatible con FHSS
* 🖥️ Dos pantallas OLED independientes
* 💡 Indicadores LED
* 🔊 Alertas mediante buzzer
* 🖥️ Diagnóstico mediante puerto serie a **115200 baudios**

---

## 🧩 Componentes de Hardware

| Componente                |        Cantidad | Función                                     |
| ------------------------- | --------------: | ------------------------------------------- |
| ESP32                     |               1 | Unidad principal de procesamiento           |
| RX5808                    |               1 | Receptor de video FPV analógico de 5.8 GHz  |
| NRF24L01                  |               1 | Detector de actividad RF en 2.4 GHz         |
| OLED SSD1306 128×64       |               2 | Visualización de información del sistema    |
| LED verde                 |               1 | Indicador de calibración                    |
| LED rojo                  |               2 | Indicadores de detección de video y FHSS    |
| Buzzer                    |               1 | Alertas acústicas                           |
| Capacitor de 10–100 µF    |               1 | Estabilización de alimentación del NRF24L01 |
| Cables / PCB / Protoboard | Según necesidad | Montaje del hardware                        |

---

## 🧠 Arquitectura del Sistema

El detector utiliza dos subsistemas independientes de monitoreo RF:

```text
                    ┌─────────────────┐
                    │      ESP32      │
                    │                 │
                    │ Análisis RF     │
                    │ Lógica          │
                    │ Alertas / UI    │
                    └───────┬─────────┘
                            │
             ┌──────────────┴──────────────┐
             │                             │
             ▼                             ▼
      ┌─────────────┐               ┌─────────────┐
      │   RX5808    │               │  NRF24L01   │
      │             │               │             │
      │ Video FPV   │               │ RF 2.4 GHz  │
      │   5.8 GHz   │               │    / FHSS   │
      └──────┬──────┘               └──────┬──────┘
             │                             │
             ▼                             ▼
       Análisis RSSI                Energía por canal
       Media / Desv.                Análisis de patrón
       Confianza                    Detección FHSS
```

---

# 📺 Detección de Video FPV en 5.8 GHz

El subsistema RX5808 escanea frecuencias comúnmente utilizadas por sistemas de video FPV analógico.

El firmware contiene **9 bandas FPV con 8 canales cada una**, proporcionando un total de:

**9 × 8 = 72 frecuencias**

### Bandas Soportadas

| Banda |
| ----- |
| A     |
| B     |
| E     |
| F     |
| R     |
| U     |
| O     |
| L     |
| H     |

El rango configurado de frecuencias se extiende aproximadamente desde:

**5325 MHz hasta 5945 MHz**

---

## 📊 Análisis RSSI

Para cada frecuencia escaneada, el ESP32 toma múltiples muestras RSSI provenientes del RX5808.

El firmware calcula:

* RSSI promedio
* Desviación estándar RSSI
* Intensidad respecto al nivel base de ruido
* Persistencia de señal
* Confianza de detección

La salida RSSI del RX5808 se conecta al ADC del ESP32 y se lee con una resolución de **12 bits**.

```text
Rango ADC: 0 – 4095
```

---

## 🎯 Puntaje de Confianza

El sistema genera un puntaje de confianza entre:

```text
0 – 100
```

El puntaje se calcula a partir de tres factores principales:

| Parámetro            | Peso |
| -------------------- | ---: |
| Intensidad de señal  |  50% |
| Estabilidad de señal |  30% |
| Persistencia         |  20% |

Esto ayuda a reducir detecciones causadas por picos RF momentáneos o interferencias inestables.

---

## 🧪 Clasificación de Señales

El firmware puede clasificar la señal más fuerte detectada como:

```text
VIDEO_VALIDO
RUIDO/INTERFERENCIA
FUERTE_INESTABLE
```

Una señal de video válida requiere suficiente intensidad, estabilidad, persistencia y confianza.

---

## 🎚️ Calibración Automática

Al iniciar el sistema, se mide automáticamente el nivel base de ruido RSSI del entorno.

El firmware actual toma:

```text
250 muestras RSSI
```

A partir de esta línea base se generan umbrales dinámicos:

```text
Detección ON  = Línea base + Margen RSSI
Detección OFF = Detección ON - Histéresis
```

Esto permite adaptar el detector a diferentes entornos de radiofrecuencia.

> Para obtener una mejor calibración, evita utilizar transmisores FPV cercanos mientras el dispositivo está iniciando.

---

# 📡 Detección de 2.4 GHz / FHSS

El segundo subsistema utiliza un **NRF24L01** conectado al ESP32 mediante HSPI.

Escanea los canales NRF:

```text
Canal 0 → Canal 83
```

Que corresponden aproximadamente a:

```text
2400 MHz → 2483 MHz
```

El NRF24L01 utiliza su función **RPD (Received Power Detector)** para determinar si existe energía RF en cada canal.

El firmware analiza:

* Múltiples canales activos
* Actividad distribuida en el espectro
* Separación suficiente entre frecuencias activas
* Detecciones repetidas durante varios escaneos consecutivos

Estas características pueden indicar actividad de tipo **FHSS (Frequency-Hopping Spread Spectrum)**.

---

## ⚠️ Limitación de Detección de ExpressLRS

El NRF24L01 **no decodifica paquetes ExpressLRS**.

La detección se basa únicamente en la distribución de energía RF y en patrones similares a saltos de frecuencia.

Por lo tanto:

```text
FHSS detectado ≠ ELRS confirmado
```

La pantalla OLED muestra intencionalmente:

```text
ELRS?
```

en lugar de afirmar una identificación definitiva de ExpressLRS.

Otros transmisores de 2.4 GHz pueden producir patrones de actividad RF similares.

---

# 🖥️ Pantallas OLED

El proyecto utiliza **dos pantallas OLED SSD1306 de 128×64 píxeles**.

Ambas pantallas utilizan la dirección I2C:

```text
0x3C
```

Esto es posible porque el ESP32 utiliza **dos controladores I2C independientes**.

---

## OLED 1 — Monitor de Video FPV

Muestra:

* Estado de detección
* Banda FPV
* Canal
* Frecuencia
* RSSI promedio
* Desviación estándar
* Puntaje de confianza
* Clasificación de señal

Ejemplo:

```text
VIDEO MONITOR
Status: TARGET
B:R CH:4 5769MHz
mean:1450 sd:45
score:87
VIDEO_VALIDO
```

---

## OLED 2 — Monitor de 2.4 GHz

Muestra:

* Estado del NRF24L01
* Estado de detección FHSS
* Número de canales activos
* Contador de confirmación
* Rango de frecuencias detectadas

Ejemplo:

```text
2.4 GHz / FHSS
NRF24: OK
Patron: ELRS?
hits:8 conf:3/3
2403-2478 MHz
FHSS no concluyente
```

---

# 🔌 Conexiones

## RX5808

| RX5808 |   ESP32 |
| ------ | ------: |
| DATA   | GPIO 23 |
| CLK    | GPIO 18 |
| CS     |  GPIO 5 |
| RSSI   | GPIO 34 |

---

## OLED de Video

| OLED      |   ESP32 |
| --------- | ------: |
| SDA       | GPIO 21 |
| SCL       | GPIO 22 |
| Dirección |  `0x3C` |

---

## OLED de 2.4 GHz

| OLED      |   ESP32 |
| --------- | ------: |
| SDA       | GPIO 16 |
| SCL       | GPIO 17 |
| Dirección |  `0x3C` |

---

## NRF24L01

| NRF24L01 |     ESP32 |
| -------- | --------: |
| SCK      |   GPIO 14 |
| MISO     |   GPIO 12 |
| MOSI     |   GPIO 13 |
| CE       |   GPIO 26 |
| CSN      |   GPIO 27 |
| VCC      | **3.3 V** |
| GND      |       GND |

> ⚠️ **No alimentes el NRF24L01 con 5 V. Utiliza únicamente 3.3 V.**

Se recomienda colocar un **capacitor de 10–100 µF** entre VCC y GND para mejorar la estabilidad de alimentación.

---

## LEDs y Buzzer

| Dispositivo                    | GPIO ESP32 |
| ------------------------------ | ---------: |
| LED verde de calibración       |    GPIO 25 |
| LED rojo de detección de video |    GPIO 32 |
| LED rojo de detección FHSS     |    GPIO 33 |
| Buzzer                         |     GPIO 4 |

---

# 🚦 Estados de los LEDs

### 🟢 LED de Calibración

Parpadea mientras se está calibrando la línea base RSSI.

### 🔴 LED de Video

Se enciende cuando el sistema identifica un objetivo válido de video analógico.

### 🔴 LED FHSS

Se enciende cuando se detecta un patrón RF persistente compatible con FHSS.

---

# 🔊 Buzzer

El ESP32 genera la señal del buzzer mediante PWM.

Frecuencia configurada:

```text
2200 Hz
```

El buzzer proporciona alertas acústicas durante eventos como:

* Encendido del dispositivo
* Detección válida de video
* Cambio de canal FPV
* Cambio de banda FPV

---

# 📚 Requisitos de Software

El firmware está diseñado para el entorno **Arduino para ESP32**.

### Librerías Utilizadas

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <math.h>
#include <string.h>
```

La mayoría de las librerías están incluidas con el entorno Arduino para ESP32.

La principal dependencia externa es:

### RF24

Instala la librería **RF24** desde el Administrador de Librerías del Arduino IDE.

```text
Arduino IDE
   ↓
Programa
   ↓
Incluir Librería
   ↓
Administrar Bibliotecas
   ↓
Buscar: RF24
```

---

## Librería OLED

El firmware actual **no requiere una librería gráfica SSD1306 externa**.

El proyecto incluye una implementación mínima del SSD1306 directamente en el código fuente, incluyendo:

* Inicialización del SSD1306
* Manejo del framebuffer
* Dibujo de píxeles
* Renderizado de texto
* Fuente ASCII 5×7
* Actualización de pantalla mediante I2C

---

# 🚀 Instalación

### 1. Clonar el repositorio

```bash
git clone https://github.com/TU-USUARIO/TU-REPOSITORIO.git
```

### 2. Abrir el firmware

Abre:

```text
tesis.ino
```

con Arduino IDE.

### 3. Instalar soporte para ESP32

Asegúrate de tener instalado el paquete de placas ESP32 en Arduino IDE.

### 4. Instalar RF24

Instala la librería `RF24` utilizando el Administrador de Librerías.

### 5. Conectar el hardware

Realiza las conexiones de acuerdo con las tablas de conexiones anteriores.

### 6. Seleccionar la placa ESP32

Selecciona el modelo de ESP32 correspondiente desde Arduino IDE.

### 7. Compilar y cargar

Compila el firmware y súbelo al ESP32.

### 8. Abrir el Monitor Serie

Configura la velocidad en:

```text
115200
```

---

# ⚙️ Parámetros Principales de Detección

Los principales parámetros pueden modificarse directamente en el firmware.

```cpp
SCAN_INTERVAL_MS
TUNE_SETTLE_MS

RSSI_CAL_SAMPLES
RSSI_WINDOW_SAMPLES

RSSI_MARGIN
RSSI_STDDEV_MAX
RSSI_HYST

VIDEO_PERSIST_N_DEFAULT

NRF_SCAN_INTERVAL_MS
NRF_FIRST_CHANNEL
NRF_LAST_CHANNEL

FHSS_MIN_HITS
FHSS_MIN_SPREAD
FHSS_CONFIRM_SCANS
FHSS_HOLD_MS
```

Estos valores controlan la sensibilidad, estabilidad, persistencia y comportamiento general de los algoritmos de detección.

---

# 🔬 Flujo de Detección

### Video FPV Analógico

```text
Inicio
  ↓
Calibración RSSI
  ↓
Escaneo de 72 frecuencias FPV
  ↓
Obtención de muestras RSSI
  ↓
Cálculo de media + desviación estándar
  ↓
Selección de la frecuencia más fuerte
  ↓
Comparación con umbral dinámico
  ↓
Cálculo de persistencia
  ↓
Cálculo del puntaje de confianza
  ↓
Clasificación de señal
  ↓
OLED / LED / Buzzer
```

### Actividad RF en 2.4 GHz

```text
Iniciar NRF24L01
  ↓
Escanear canales 0–83
  ↓
Detectar energía mediante RPD
  ↓
Contar canales activos
  ↓
Calcular dispersión de frecuencias
  ↓
Buscar patrón FHSS
  ↓
Confirmar durante varios escaneos
  ↓
OLED / LED
```

---

# 🔐 Funcionamiento Pasivo

Este proyecto está diseñado como un **detector pasivo**.

No necesita establecer una comunicación con el sistema FPV detectado. En su lugar, observa y analiza la actividad de radiofrecuencia que ya está presente en el entorno.

El RX5808 monitorea transmisiones de video FPV analógico, mientras que el NRF24L01 analiza patrones de energía en la banda ISM de 2.4 GHz.

---

# ⚠️ Limitaciones

Este proyecto no debe considerarse un sistema universal de identificación de drones.

El rendimiento de detección puede verse afectado por:

* Interferencias RF
* Redes Wi-Fi
* Dispositivos Bluetooth
* Otros transmisores de 2.4 GHz
* Transmisores de video analógico cercanos
* Tipo y orientación de las antenas
* Sensibilidad de los receptores
* Distancia al transmisor
* Ruido electromagnético del entorno

El subsistema NRF24L01 detecta energía RF y patrones similares a FHSS, pero no puede determinar con certeza qué protocolo o dispositivo los está generando.

---

# 🛠️ Posibles Mejoras Futuras

Algunas mejoras posibles incluyen:

* Mejores filtros RF
* Antenas direccionales
* Registro histórico de señales
* Almacenamiento en tarjeta SD
* Panel web
* Interfaz de monitoreo por Wi-Fi
* Estadísticas de detección
* Visualización del espectro
* Integración con GPS
* Múltiples estaciones receptoras
* Registro de fecha y hora de eventos
* Algoritmos más avanzados de clasificación RF

---

# 🤝 Contribuciones

Las contribuciones son bienvenidas.

Puedes colaborar mediante:

* Reporte de errores
* Mejoras en los algoritmos de detección
* Pruebas en distintos entornos RF
* Mejoras en la documentación
* Soporte para nuevo hardware
* Optimización del rendimiento de escaneo
* Mejoras de la interfaz de usuario

Puedes abrir un **Issue** o enviar un **Pull Request**.

---

# 📜 Licencia

Este proyecto se distribuye bajo la **Licencia MIT**.

Puedes utilizar, modificar, distribuir y mejorar el proyecto de acuerdo con los términos de dicha licencia.

Consulta el archivo `LICENSE` para más información.

---

# ⚠️ Aviso Legal

Este proyecto está destinado a fines de **investigación, educación, experimentación y monitoreo de radiofrecuencia**.

Los usuarios son responsables de garantizar que el uso del proyecto cumpla con todas las leyes y regulaciones aplicables en su jurisdicción.

Los autores y colaboradores no se hacen responsables del uso indebido del software o del hardware.

---

## ⭐ Apoya el Proyecto

Si este proyecto te resulta útil, puedes darle una **estrella ⭐ al repositorio**.

Las contribuciones, pruebas, mejoras de documentación y comentarios son bienvenidos.
