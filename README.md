# passive-drone-detection
Passive system for detecting and monitoring FPV drones and UAVs through radio-frequency signals. It scans analog video channels, analyzes signal strength, and generates alerts when a transmission is detected or lost.

This project implements a passive radio-frequency detection system for FPV drones, using an ESP32 as the main processing unit.

The system combines two monitoring methods. The first uses an RX5808 receiver to scan analog video channels in the approximately 5.3–5.9 GHz range. The ESP32 analyzes the receiver's RSSI signal, performs automatic noise-floor calibration, and calculates parameters such as average signal strength, standard deviation, persistence, and a confidence score to help distinguish possible video transmissions from noise or interference.

The second method uses an NRF24L01 to monitor radio-frequency activity in the 2.4 GHz band. The system scans available channels and looks for energy patterns distributed across multiple frequencies that may be compatible with FHSS (Frequency-Hopping Spread Spectrum) communications.

Important: NRF24L01-based detection indicates RF activity and patterns compatible with FHSS, but it does not uniquely identify the ExpressLRS (ELRS) protocol.

System status is displayed using two independent SSD1306 OLED displays. LEDs and a buzzer are also used to provide visual and audible alerts.

Hardware Components
1× ESP32
Main microcontroller.
Processes RSSI data and controls the receivers, displays, LEDs, and buzzer.
1× RX5808
5.8 GHz analog video receiver.
Used to scan FPV bands A, B, E, F, R, U, O, L, and H.
Provides an analog RSSI output to the ESP32.
1× NRF24L01
2.4 GHz transceiver used in receive mode for RF activity detection.
Communicates through SPI.
Uses the RPD function to detect energy in the 2.4 GHz channels.
Must be powered with 3.3 V only.
1× 10–100 µF capacitor
Recommended between VCC and GND of the NRF24L01 to improve power stability.
2× SSD1306 128×64 I2C OLED displays
Configured address: 0x3C.
Each display uses an independent I2C bus, allowing both displays to use the same address.
One displays FPV video detection information.
The other displays 2.4 GHz/FHSS scanning information.
1× Green LED
RSSI calibration indicator.
2× Red LEDs
One indicates valid analog video detection.
One indicates detection of an FHSS-compatible pattern.
1× Buzzer
Generates audible alerts when specific events are detected.
Wires, breadboard/PCB, power supply, and other required connection components.
Pin Connections
RX5808
DATA → GPIO 23
CLK → GPIO 18
CS → GPIO 5
RSSI → GPIO 34
OLED — Video Monitor
SDA → GPIO 21
SCL → GPIO 22
I2C Address → 0x3C
OLED — 2.4 GHz/FHSS Monitor
SDA → GPIO 16
SCL → GPIO 17
I2C Address → 0x3C
NRF24L01
SCK → GPIO 14
MISO → GPIO 12
MOSI → GPIO 13
CE → GPIO 26
CSN → GPIO 27
VCC → 3.3 V
Indicators
Calibration LED → GPIO 25
Video Detection LED → GPIO 32
FHSS Detection LED → GPIO 33
Buzzer → GPIO 4
Libraries Used

The firmware uses the following libraries:

Arduino.h
Wire.h
SPI.h
RF24.h
math.h
string.h

The main external library required is RF24, used to control the NRF24L01 module.

Main Features
Automatic scanning of 72 FPV frequencies across 9 bands.
RSSI measurement using a 12-bit ADC.
Automatic noise-floor calibration.
Dynamic detection thresholds.
Hysteresis to reduce false signal-loss events.
RSSI mean and standard deviation calculation.
Detection persistence system.
Confidence score from 0–100.
Classification between valid signals, interference, and unstable strong signals.
Channel and band change tracking.
RF activity scanning from approximately 2400 to 2483 MHz using the NRF24L01.
Analysis of patterns compatible with FHSS.
Confirmation through multiple consecutive scans.
Two independent OLED interfaces.
LED status indicators.
Buzzer alerts.
Serial monitoring and diagnostics at 115200 baud.

The system operates passively, as its purpose is to observe and analyze radio-frequency activity already present in the environment without requiring communication with the detected drone.
