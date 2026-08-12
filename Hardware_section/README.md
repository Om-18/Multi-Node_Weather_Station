# Hardware Section - Multi-Node Weather Station

## Overview
This folder contains the firmware and documentation for the hardware components of the Multi-Node Weather Station project. The system consists of two ESP32 microcontroller units communicating via LoRa technology:
- **Sender Node**: ESP32-WROOM-32 with integrated sensors
- **Receiver Node**: ESP32-S3 as a data collection hub

---

## System Architecture

### Sender Node (ESP32-WROOM-32)
The sender node collects environmental and motion data from multiple sensors and transmits it wirelessly to the receiver.

**Microcontroller**: ESP32-WROOM-32

**Sensors**:
| Sensor | Model | Interface | Function |
|--------|-------|-----------|----------|
| Pressure/Altitude | Adafruit DPS310 | I2C | Measures atmospheric pressure |
| Environmental | Adafruit BME680 | I2C | Measures temperature, humidity, pressure, and VOC |
| Motion | MPU6050 | I2C | Accelerometer and gyroscope |
| Magnetometer | TLV493D | I2C | Measures magnetic field |
| LoRa Module | Generic LoRa | UART2 | Wireless communication |

**I2C Configuration**:
- SDA: GPIO 21
- SCL: GPIO 22
- Clock Speed: 100 kHz

**LoRa Configuration**:
- UART: UART2
- TX: GPIO 16
- RX: GPIO 17
- Baud Rate: 115200
- Address: 1
- Network ID: 10

### Receiver Node (ESP32-S3)
The receiver node listens for incoming data from sender nodes and parses/displays the information.

**Microcontroller**: ESP32-S3

**Communication**:
- LoRa Module: UART1
- TX: GPIO 17
- RX: GPIO 18
- Baud Rate: 115200
- Address: 2
- Network ID: 10

---

## Firmware Files

### Sender_Code_ESP32_wroom_32.ino
Main sender firmware that:
- Initializes all sensors via I2C
- Collects environmental and motion data
- Formats and transmits data via LoRa

**Key Components**:
- Sensor initialization in `setup()`
- Data collection loop in `loop()`
- Automatic serial communication for debugging

### Receiver_Code_ESP32_S3.ino
Receiver firmware that:
- Listens on LoRa for incoming messages
- Parses comma-separated data values
- Outputs received data to serial monitor

**Key Components**:
- LoRa module configuration
- Data reception and parsing logic
- Serial output for monitoring

---

## Hardware Connections

### Sender (ESP32-WROOM-32) Pinout

```
I2C Bus (All Sensors):
  GPIO 21 (SDA)
  GPIO 22 (SCL)

LoRa Module (UART2):
  GPIO 16 (TX)
  GPIO 17 (RX)
  GND, +3.3V/+5V (depends on module)

USB:
  GPIO for Serial debugging (USB-UART via onboard chip)
```

### Receiver (ESP32-S3) Pinout

```
LoRa Module (UART1):
  GPIO 17 (RX)
  GPIO 18 (TX)
  GND, +3.3V/+5V (depends on module)

USB:
  GPIO for Serial debugging (USB-UART via onboard chip)
```

---

## Setup Instructions

### 1. Hardware Assembly
- Connect all sensors to the sender node via I2C (GPIO 21 & 22)
- Connect LoRa modules to both nodes via their respective UART ports
- Ensure proper power supply (3.3V for ESP32 logic, 5V for some sensors if needed)
- Verify all connections before powering on

### 2. Software Setup

1. **Install Arduino IDE** (if not already installed)
2. **Add ESP32 Board Support**:
   - Go to Preferences → Additional Board Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board Manager → Search "ESP32" → Install

3. **Install Required Libraries**:
   ```
   - Adafruit DPS310
   - Adafruit BME680
   - Adafruit Unified Sensor
   - Tlv493d (or equivalent magnetometer library)
   ```

4. **Configure Board Settings**:
   - Sender: Select "ESP32 Dev Module" or equivalent WROOM-32
   - Receiver: Select "ESP32-S3"
   - Set appropriate Serial port and baud rate (115200)

5. **Upload Firmware**:
   - Upload `Sender_Code_ESP32_wroom_32.ino` to the sender node
   - Upload `Receiver_Code_ESP32_S3.ino` to the receiver node

### 3. Testing & Verification

1. Open Serial Monitor (115200 baud) on both devices
2. Sender should display initialization messages
3. Receiver should display received data in the format:
   ```
   RAW:
   [raw LoRa message]
   Parsed Data:
   [extracted sensor data]
   ```
4. Check for "No data Received" messages on receiver if connectivity issues occur

---

## Data Communication Format

### LoRa Message Structure
Messages are transmitted as comma-separated values:
```
ADDR,NETID,SEQ,[sensor_data]
```

The receiver skips the first three fields and extracts the sensor data portion.

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| LoRa not connecting | Verify address and network ID match on both nodes |
| Sensor read failures | Check I2C connections (SDA/SCL) and pull-up resistors |
| Serial output garbled | Verify baud rate is set to 115200 |
| No data received | Check LoRa module power supply and antenna connections |
| I2C address conflicts | Use I2C scanner to identify addresses and adjust wire addresses in code |

---

## Power Consumption
- **Idle**: ~80 mA
- **Active (sensors + LoRa TX)**: ~200-400 mA (varies with transmission power)
- **Recommended Power Supply**: 5V USB or battery pack with proper voltage regulation

---

## Future Enhancements
- Add energy harvesting capabilities
- Implement sleep modes for battery operation
- Expand to multiple sender nodes
- Add data logging to SD card
- Implement edge processing/data filtering

---

## References
- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [Adafruit Sensor Libraries Documentation](https://learn.adafruit.com)
- LoRa Module Documentation (refer to module manufacturer)

---

## Support
For issues or questions, refer to the main project documentation or contact the development team.
