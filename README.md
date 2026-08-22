# passive-drone-detection
Passive system for detecting and monitoring FPV drones and UAVs through radio-frequency signals. It scans analog video channels, analyzes signal strength, and generates alerts when a transmission is detected or lost.

An **ESP32-based passive RF monitoring system** designed to detect radio-frequency activity associated with FPV drones.

The project combines **5.8 GHz analog video detection** using an RX5808 receiver with **2.4 GHz RF activity monitoring** using an NRF24L01. The system analyzes signal strength, stability, persistence, and frequency activity to identify potential FPV video transmissions and FHSS-like radio activity.

> **Note:** The 2.4 GHz detector identifies RF activity and frequency-hopping patterns compatible with FHSS. It does **not** uniquely or cryptographically identify ExpressLRS (ELRS).

---

## ✨ Features

* 📡 Passive RF monitoring
* 📺 Analog FPV video detection
* 📶 2.4 GHz RF activity monitoring
* 🔍 Automatic scanning of **72 FPV frequencies**
* 📊 RSSI signal analysis
* 📈 Mean and standard deviation calculation
* 🎯 Dynamic detection thresholds
* 🔄 Automatic RSSI noise-floor calibration
* 🧠 Signal confidence score from **0 to 100**
* ⏱️ Detection persistence analysis
* 🔒 Signal lock/unlock hysteresis
* 🚨 Channel and band change detection
* 📡 FHSS-compatible activity detection
* 🖥️ Two independent OLED displays
* 💡 LED status indicators
* 🔊 Buzzer alerts
* 🖥️ Serial debugging at **115200 baud**

---

## 🧩 Hardware

| Component                |    Quantity | Purpose                             |
| ------------------------ | ----------: | ----------------------------------- |
| ESP32                    |           1 | Main processing unit                |
| RX5808                   |           1 | 5.8 GHz analog FPV receiver         |
| NRF24L01                 |           1 | 2.4 GHz RF activity detector        |
| SSD1306 128×64 OLED      |           2 | Real-time system information        |
| Green LED                |           1 | Calibration indicator               |
| Red LED                  |           2 | Video and FHSS detection indicators |
| Buzzer                   |           1 | Audible detection alerts            |
| 10–100 µF capacitor      |           1 | NRF24L01 power stabilization        |
| Wires / PCB / Breadboard | As required | Hardware assembly                   |

---

## 🧠 System Architecture

The detector uses two independent RF monitoring subsystems:

```text
                    ┌─────────────────┐
                    │      ESP32      │
                    │                 │
                    │ Signal Analysis │
                    │ Detection Logic │
                    │ Alerts / UI     │
                    └───────┬─────────┘
                            │
             ┌──────────────┴──────────────┐
             │                             │
             ▼                             ▼
      ┌─────────────┐               ┌─────────────┐
      │   RX5808    │               │  NRF24L01   │
      │             │               │             │
      │ 5.8 GHz FPV │               │  2.4 GHz RF │
      │    Video    │               │    / FHSS   │
      └──────┬──────┘               └──────┬──────┘
             │                             │
             ▼                             ▼
      RSSI Analysis                 Channel Energy
      Mean / StdDev                 Pattern Analysis
      Confidence                    FHSS Detection
```

---

# 📺 5.8 GHz FPV Video Detection

The RX5808 subsystem scans common analog FPV video frequencies.

The firmware contains **9 FPV bands with 8 channels each**, providing a total of:

**9 × 8 = 72 frequencies**

### Supported Bands

| Band |
| ---- |
| A    |
| B    |
| E    |
| F    |
| R    |
| U    |
| O    |
| L    |
| H    |

The configured frequency range extends approximately from:

**5325 MHz to 5945 MHz**

---

## 📊 RSSI Analysis

For every scanned frequency, the ESP32 collects multiple RSSI samples from the RX5808.

The firmware calculates:

* Mean RSSI
* RSSI standard deviation
* Signal strength relative to the noise floor
* Signal persistence
* Detection confidence

The RX5808 RSSI output is connected to the ESP32 ADC and read with **12-bit resolution**.

```text
ADC Range: 0 – 4095
```

---

## 🎯 Confidence Score

The system generates a confidence score between:

```text
0 – 100
```

The score is calculated from three main factors:

| Parameter        | Weight |
| ---------------- | -----: |
| Signal intensity |    50% |
| Signal stability |    30% |
| Persistence      |    20% |

This helps reduce detections caused by short RF spikes or unstable interference.

---

## 🧪 Signal Classification

The firmware can classify the strongest detected signal as:

```text
VIDEO_VALIDO
RUIDO/INTERFERENCIA
FUERTE_INESTABLE
```

A valid video signal requires sufficient signal strength, stability, persistence, and confidence.

---

## 🎚️ Automatic Calibration

At startup, the system automatically measures the local RSSI noise floor.

The current firmware takes:

```text
250 RSSI samples
```

From this baseline, dynamic thresholds are generated:

```text
Detection ON  = Baseline + RSSI Margin
Detection OFF = Detection ON - Hysteresis
```

This allows the detector to adapt to different RF environments.

> For better calibration, avoid operating a nearby FPV transmitter while the device is starting.

---

# 📡 2.4 GHz / FHSS Detection

The second detection subsystem uses an **NRF24L01** connected to the ESP32 through HSPI.

It scans NRF channels:

```text
Channel 0 → Channel 83
```

Corresponding approximately to:

```text
2400 MHz → 2483 MHz
```

The NRF24L01 uses its **RPD (Received Power Detector)** function to determine whether RF energy is present on each channel.

The firmware looks for:

* Multiple active channels
* Activity distributed across the spectrum
* Sufficient frequency spread
* Repeated detections over consecutive scans

These characteristics may indicate **frequency-hopping spread spectrum (FHSS)** activity.

---

## ⚠️ ExpressLRS Detection Limitation

The NRF24L01 detector does **not decode ExpressLRS packets**.

Detection is based only on RF energy distribution and hopping-like patterns.

Therefore:

```text
FHSS detected ≠ ELRS confirmed
```

The OLED intentionally displays:

```text
ELRS?
```

rather than claiming definitive ELRS identification.

Other 2.4 GHz transmitters may produce similar RF activity.

---

# 🖥️ OLED Displays

The project uses **two SSD1306 128×64 OLED displays**.

Both displays use I2C address:

```text
0x3C
```

This is possible because the ESP32 uses **two independent I2C controllers**.

---

## OLED 1 — FPV Video Monitor

Displays:

* Detection status
* FPV band
* Channel
* Frequency
* Mean RSSI
* RSSI standard deviation
* Confidence score
* Signal classification

Example:

```text
VIDEO MONITOR
Status: TARGET
B:R CH:4 5769MHz
mean:1450 sd:45
score:87
VIDEO_VALIDO
```

---

## OLED 2 — 2.4 GHz Monitor

Displays:

* NRF24L01 status
* FHSS detection state
* Number of active channels
* Confirmation counter
* Detected frequency spread

Example:

```text
2.4 GHz / FHSS
NRF24: OK
Patron: ELRS?
hits:8 conf:3/3
2403-2478 MHz
FHSS no concluyente
```

---

# 🔌 Pinout

## RX5808

| RX5808 |   ESP32 |
| ------ | ------: |
| DATA   | GPIO 23 |
| CLK    | GPIO 18 |
| CS     |  GPIO 5 |
| RSSI   | GPIO 34 |

---

## Video OLED

| OLED    |   ESP32 |
| ------- | ------: |
| SDA     | GPIO 21 |
| SCL     | GPIO 22 |
| Address |  `0x3C` |

---

## 2.4 GHz OLED

| OLED    |   ESP32 |
| ------- | ------: |
| SDA     | GPIO 16 |
| SCL     | GPIO 17 |
| Address |  `0x3C` |

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

> ⚠️ **Do not power the NRF24L01 from 5 V. Use 3.3 V only.**

A **10–100 µF capacitor** between VCC and GND is recommended to improve power stability.

---

## LEDs and Buzzer

| Device                  | ESP32 GPIO |
| ----------------------- | ---------: |
| Green calibration LED   |    GPIO 25 |
| Red video detection LED |    GPIO 32 |
| Red FHSS detection LED  |    GPIO 33 |
| Buzzer                  |     GPIO 4 |

---

# 🚦 LED Status

### 🟢 Calibration LED

Blinks while the RSSI baseline is being calibrated.

### 🔴 Video LED

Turns on when the system identifies a valid analog video target.

### 🔴 FHSS LED

Turns on when a persistent FHSS-compatible RF pattern is detected.

---

# 🔊 Buzzer

The ESP32 generates the buzzer signal using PWM.

Configured tone:

```text
2200 Hz
```

The buzzer provides audible feedback during events such as:

* Device startup
* Valid video detection
* FPV channel changes
* FPV band changes

---

# 📚 Software Requirements

The firmware is designed for the **ESP32 Arduino framework**.

### Included Libraries

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <math.h>
#include <string.h>
```

Most libraries are included with the ESP32 Arduino environment.

The main external dependency is:

### RF24

Install the **RF24** library using the Arduino IDE Library Manager.

```text
Arduino IDE
   ↓
Sketch
   ↓
Include Library
   ↓
Manage Libraries
   ↓
Search: RF24
```

---

## OLED Library

No external SSD1306 graphics library is required by the current firmware.

The project contains a minimal SSD1306 implementation directly in the source code, including:

* SSD1306 initialization
* Frame buffer handling
* Pixel drawing
* Text rendering
* 5×7 ASCII font
* I2C display updates

---

# 🚀 Installation

### 1. Clone the repository

```bash
git clone https://github.com/YOUR-USERNAME/YOUR-REPOSITORY.git
```

### 2. Open the firmware

Open:

```text
tesis.ino
```

with the Arduino IDE.

### 3. Install ESP32 support

Make sure your Arduino IDE has ESP32 board support installed.

### 4. Install RF24

Install the `RF24` library using the Arduino Library Manager.

### 5. Connect the hardware

Wire the components according to the pinout tables above.

### 6. Select your ESP32 board

Select the appropriate ESP32 board from the Arduino IDE.

### 7. Compile and upload

Compile the firmware and upload it to the ESP32.

### 8. Open Serial Monitor

Set the baud rate to:

```text
115200
```

---

# ⚙️ Main Detection Parameters

The main detection parameters can be adjusted directly in the firmware.

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

These parameters control the sensitivity, stability, persistence, and behavior of the detection algorithms.

---

# 🔬 Detection Workflow

### Analog FPV Video

```text
Start
  ↓
RSSI Calibration
  ↓
Scan 72 FPV Frequencies
  ↓
Collect RSSI Samples
  ↓
Calculate Mean + Standard Deviation
  ↓
Select Strongest Frequency
  ↓
Compare Against Dynamic Threshold
  ↓
Calculate Persistence
  ↓
Calculate Confidence Score
  ↓
Classify Signal
  ↓
Display / LED / Buzzer
```

### 2.4 GHz RF Activity

```text
Start NRF24L01
  ↓
Scan Channels 0–83
  ↓
Test RPD Energy
  ↓
Count Active Channels
  ↓
Calculate Frequency Spread
  ↓
Check FHSS Pattern
  ↓
Confirm Across Multiple Scans
  ↓
Display / LED Alert
```

---

# 🔐 Passive Operation

This project is designed as a **passive detector**.

It does not need to establish a communication link with the detected FPV system. Instead, it observes RF activity already present in the environment.

The RX5808 monitors analog FPV video transmissions while the NRF24L01 monitors energy patterns in the 2.4 GHz ISM band.

---

# ⚠️ Limitations

This project should not be considered a universal drone identification system.

Detection performance can be affected by:

* RF interference
* Wi-Fi networks
* Bluetooth devices
* Other 2.4 GHz transmitters
* Nearby analog video transmitters
* Antenna type and orientation
* Receiver sensitivity
* Distance from the transmitter
* Local electromagnetic noise

The NRF24L01 subsystem detects RF energy and FHSS-like patterns but cannot determine with certainty which protocol or device generated them.

---

# 🛠️ Possible Future Improvements

Potential improvements include:

* Better RF front-end filtering
* Directional antennas
* Signal history logging
* SD card logging
* Web-based dashboard
* Wi-Fi monitoring interface
* Detection statistics
* Spectrum visualization
* GPS integration
* Multiple receiving stations
* Detection event timestamps
* More advanced RF classification

---

# 🤝 Contributing

Contributions are welcome.

You can contribute by:

* Reporting bugs
* Improving detection algorithms
* Testing different RF environments
* Improving documentation
* Adding hardware support
* Optimizing scan performance
* Improving the user interface

Feel free to open an **Issue** or submit a **Pull Request**.

---

# 📜 License

This project is released under the **MIT License**.

You are free to use, modify, distribute, and improve the project according to the terms of the license.

See the `LICENSE` file for more information.

---

# ⚠️ Disclaimer

This project is intended for **research, education, experimentation, and RF monitoring**.

Users are responsible for ensuring that their use of the project complies with all applicable local laws and regulations.

The authors and contributors are not responsible for misuse of the software or hardware.

---

## ⭐ Support the Project

If you find this project useful, consider giving the repository a **star ⭐**.

Contributions, testing, documentation improvements, and feedback are welcome.
