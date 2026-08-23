# passive-drone-detection
Passive system for detecting and monitoring FPV drones and UAVs through radio-frequency signals. It scans analog video channels, analyzes signal strength, and generates alerts when a transmission is detected or lost.

An **ESP32-based passive RF monitoring system** designed to detect radio-frequency activity associated with FPV drones.

The project combines **5.8 GHz analog video detection** using an RX5808 receiver with **2.4 GHz RF activity monitoring** using an NRF24L01. The system analyzes signal strength, stability, persistence, and frequency activity to identify potential FPV video transmissions and FHSS-like radio activity.

> **Note:** The 2.4 GHz detector identifies RF activity and frequency-hopping patterns compatible with FHSS. It does **not** uniquely or cryptographically identify ExpressLRS (ELRS).

---

## Quick Start

1. Assemble the hardware according to the wiring and pinout sections below.
2. Install **ESP32 board support** in the Arduino IDE.
3. Install the **RF24** library using the Arduino Library Manager.
4. Open the project firmware (`.ino`) in the Arduino IDE.
5. Select the correct ESP32 board and serial port.
6. Compile and upload the firmware.
7. Open the Serial Monitor at **115200 baud**.
8. Power on the detector without a nearby FPV transmitter and allow the RSSI baseline calibration to complete.
9. Power on an analog FPV transmitter and observe the video-monitor OLED, LEDs, and buzzer.
10. Optionally connect the RX5808 **VIDEO / AV output** to a compatible composite monitor to view the received analog video in real time.

> **Important:** Power the NRF24L01 from **3.3 V only**. A **10–100 µF capacitor** between VCC and GND is recommended for power stability.

---

## How Detection Works

The detector combines two independent passive RF monitoring paths:

```text
                  Passive Drone Detection
                           │
             ┌─────────────┴─────────────┐
             │                           │
             ▼                           ▼
      ┌─────────────┐             ┌─────────────┐
      │   5.8 GHz   │             │   2.4 GHz   │
      │   RX5808    │             │  NRF24L01   │
      └──────┬──────┘             └──────┬──────┘
             │                           │
        FPV frequency                RF channel
          scanning                    scanning
             │                           │
        RSSI samples                  RPD energy
             │                           │
      Mean / StdDev /              Active channels
        persistence                + frequency spread
             │                           │
      Confidence score              FHSS-like pattern
             │                           │
             └─────────────┬─────────────┘
                           ▼
                     Detection logic
                           │
              ┌────────────┼────────────┐
              ▼            ▼            ▼
             OLED          LED        Buzzer
```

The **RX5808 path** scans analog FPV video frequencies and evaluates RSSI strength, stability, persistence, and confidence. The **NRF24L01 path** scans the 2.4 GHz band for distributed RF activity and frequency spread that may indicate an FHSS-like transmission.

Both paths are passive: the system observes existing RF activity and does not need to establish a communication link with the detected transmitter.

---

## Features

*  Passive RF monitoring
*  Analog FPV video detection
*  2.4 GHz RF activity monitoring
*  Automatic scanning of **72 FPV frequencies**
*  RSSI signal analysis
*  Mean and standard deviation calculation
*  Dynamic detection thresholds
*  Automatic RSSI noise-floor calibration
*  Signal confidence score from **0 to 100**
*  Detection persistence analysis
*  Signal lock/unlock hysteresis
*  Channel and band change detection
*  FHSS-compatible activity detection
*  Two independent OLED displays
*  LED status indicators
*  Buzzer alerts
*  Serial debugging at **115200 baud**

---

## Hardware

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

### Component Gallery

<p align="center"><sub>Click an image to open the corresponding component page.</sub></p>

<table>
  <tr>
    <td align="center" width="25%">
      <a href="https://www.aliexpress.com/item/1005008433033497.html">
        <img src="https://wrekd.com/cdn/shop/files/RX5808_1800x1800.jpg?v=1758237563" alt="RX5808" width="140"><br>
        <b>RX5808</b>
      </a>
    </td>
    <td align="center" width="25%">
      <a href="https://joy-it.net/en/products/SBC-NodeMCU-ESP32-C">
        <img src="https://joy-it.net/assets/images/f/SBC-NodeMCU-ESP32-C_01-1pdnt02nkz1ces7.png" alt="ESP32" width="140"><br>
        <b>ESP32</b>
      </a>
    </td>
    <td align="center" width="25%">
      <a href="https://hubot.cl/producto/transceptor-rf-nrf24l01-2-4-ghz-con-antena-sku-638h1/">
        <img src="https://hubot.cl/wp-content/uploads/2020/03/638-1-150x150.jpg" alt="NRF24L01" width="140"><br>
        <b>NRF24L01</b>
      </a>
    </td>
    <td align="center" width="25%">
      <a href="https://naylampmechatronics.com/oled/850-display-oled-096-i2c-12864-ssd1306.html">
        <img src="https://naylampmechatronics.com/3159-superlarge_default/display-oled-096-i2c-12864-ssd1306.jpg" alt="SSD1306 128x64 OLED" width="140"><br>
        <b>SSD1306 128×64 OLED</b>
      </a>
    </td>
  </tr>
  <tr>
    <td align="center" width="25%">
      <a href="https://cl.rsdelivers.com/product/kingbright/l-813gd/led-kingbright-l-813-verde-568nm-vf-22-v-60-l-10-2/0590187">
        <img src="https://static.rapidonline.com/catalogueimages/product/55/03/s55-0305p01ws.jpg" alt="Green LED" width="140"><br>
        <b>Green LED</b>
      </a>
    </td>
    <td align="center" width="25%">
      <a href="https://cl.rsdelivers.com/product/lite-on/ltl-307e/led-lite-on-ltl-rojo-623nm-vf-26-v-50-orificio-5-2/1689439">
        <img src="https://media.rs-online.com/f_auto%2Cq_auto%2Cbo_1.5px_solid_white%2Cb_auto%2Cc_pad%2Cdpr_2%2Ch_399%2Cw_710/c_pad%2Ch_399%2Cw_710/R1278392-01?pgw=1" alt="Red LED" width="140"><br>
        <b>Red LED</b>
      </a>
    </td>
    <td align="center" width="25%">
      <a href="https://www.electroardu.cl/buzzer-pasivo-5v">
        <img src="https://cdnx.jumpseller.com/electroardu/image/42270382/resize/540/540?1700319331=" alt="Buzzer" width="140"><br>
        <b>Buzzer</b>
      </a>
    </td>
    <td align="center" width="25%">
      <a href="https://www.mercadolibre.cl/bby-capacitor-electrolitico-100-uf-63v-10-piezas/p/MLC2095994835">
        <img src="https://www.cdiscount.com/pdt2/5/1/0/1/550x550/aaajk53510/rw/condensateur-100uf-63v.jpg" alt="10-100 uF capacitor" width="140"><br>
        <b>10–100 µF capacitor</b>
      </a>
    </td>
  </tr>
</table>

---

## Wiring Overview

The following overview summarizes the main hardware connections. The detailed pinout tables are provided later in this README.

### RX5808 → ESP32

```text
RX5808 DATA  ─────────────► GPIO 23
RX5808 CLK   ─────────────► GPIO 18
RX5808 CS    ─────────────► GPIO 5
RX5808 RSSI  ─────────────► GPIO 34

RX5808 VIDEO / AV OUT ────► Composite AV monitor / display (optional)
RX5808 GND            ────► AV monitor GND
```

### NRF24L01 → ESP32

```text
NRF24L01 SCK   ───────────► GPIO 14
NRF24L01 MISO  ───────────► GPIO 12
NRF24L01 MOSI  ───────────► GPIO 13
NRF24L01 CE    ───────────► GPIO 26
NRF24L01 CSN   ───────────► GPIO 27
NRF24L01 VCC   ───────────► 3.3 V
NRF24L01 GND   ───────────► GND
```

```text
          10–100 µF
3.3 V ──────||────── GND
       NRF24L01 supply
```

> **Warning:** Do **not** power the NRF24L01 from 5 V. Use **3.3 V only**.

### OLED Displays → ESP32

```text
Video OLED
SDA ──────────────────────► GPIO 21
SCL ──────────────────────► GPIO 22
I2C address ──────────────► 0x3C

2.4 GHz OLED
SDA ──────────────────────► GPIO 16
SCL ──────────────────────► GPIO 17
I2C address ──────────────► 0x3C
```

### Status LEDs and Buzzer

```text
Green calibration LED ────► GPIO 25
Red video detection LED ──► GPIO 32
Red FHSS detection LED ───► GPIO 33
Buzzer ───────────────────► GPIO 4
```

---

## System Architecture

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

# 5.8 GHz FPV Video Detection

The RX5808 subsystem scans common analog FPV video frequencies.

## Optional Live Video Output

If the RX5808 module exposes its **VIDEO / AV output**, it can be connected directly to a compatible **composite AV monitor or display** to watch the received analog FPV video in real time.

```text
RX5808 VIDEO / AV OUT  ─────►  Composite AV monitor / display
RX5808 GND             ─────►  AV monitor GND
```

This connection is optional and independent from the ESP32 detection logic. The ESP32 uses the RX5808 control and RSSI signals for scanning and detection, while the external AV display can be used to visually inspect the video signal on the currently tuned channel.

> **Note:** The exact video-output pin name and pinout can vary between RX5808 boards or breakout modules. Verify the pinout of your specific module before connecting an AV display.

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

## RSSI Analysis

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

## Confidence Score

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

## Signal Classification

The firmware can classify the strongest detected signal as:

```text
VALID_VIDEO
NOISE/INTERFERENCE
STRONG_UNSTABLE
```

A valid video signal requires sufficient signal strength, stability, persistence, and confidence.

---

## Automatic Calibration

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

# 2.4 GHz / FHSS Detection

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

## ExpressLRS Detection Limitation

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

# OLED Displays

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
VALID_VIDEO
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
Pattern: ELRS?
hits:8 conf:3/3
2403-2478 MHz
FHSS inconclusive
```

---

# Pinout

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

**Do not power the NRF24L01 from 5 V. Use 3.3 V only.**

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

# LED Status

### 🟢 Calibration LED

Blinks while the RSSI baseline is being calibrated.

### 🔴 Video LED

Turns on when the system identifies a valid analog video target.

### 🔴 FHSS LED

Turns on when a persistent FHSS-compatible RF pattern is detected.

---

# Buzzer

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

# Software Requirements

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

# Installation

### 1. Clone the repository

```bash
git clone https://github.com/Bit101-git/passive_drone_detection-PDD.git
```

### 2. Open the firmware

Open:

```text
V1-analog_video_5.8Ghz.ino
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

# Main Detection Parameters

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

# Detection Workflow

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

# Passive Operation

This project is designed as a **passive detector**.

It does not need to establish a communication link with the detected FPV system. Instead, it observes RF activity already present in the environment.

The RX5808 monitors analog FPV video transmissions while the NRF24L01 monitors energy patterns in the 2.4 GHz ISM band.

---

# Limitations

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

# Possible Future Improvements

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

# Contributing

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

# License

This project is released under the **MIT License**.

You are free to use, modify, distribute, and improve the project according to the terms of the license.

See the `LICENSE` file for more information.

---

# Disclaimer

This project is intended for **research, education, experimentation, and RF monitoring**.

Users are responsible for ensuring that their use of the project complies with all applicable local laws and regulations.

The authors and contributors are not responsible for misuse of the software or hardware.

---

## Support the Project

If you find this project useful, consider giving the repository a **star ⭐**.

Contributions, testing, documentation improvements, and feedback are welcome.
