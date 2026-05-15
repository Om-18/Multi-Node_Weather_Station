# Weather Dashboard

A web dashboard that connects to your ESP32-S3 receiver over HTTP and displays live sensor values.

## What it shows
- Temperature
- Pressure
- Humidity
- Gas Resistance
- Magnetic Field

## Setup

### 1. Update Receiver Firmware
Before running the dashboard, update the receiver ESP32-S3 with the new HTTP-enabled firmware:

1. Open `Receiver_Code_ESP32_S3.ino` in Arduino IDE
2. Update WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_SSID";
   const char* password = "YOUR_PASSWORD";
   ```
3. Install required libraries:
   - `WiFi` (built-in)
   - `WebServer` (built-in)
   - `ArduinoJson` (install via Library Manager)
4. Upload the firmware to your ESP32-S3 receiver
5. Note the receiver's IP address from the Serial Monitor

### 2. Run the Dashboard
1. Open a terminal in `Dashboard/`
2. Run `npm install`
3. Run `npm start`
4. Open `http://localhost:3000` in your browser
5. Enter the receiver's IP address (e.g., `192.168.1.100`)
6. Click **Connect**

## API Endpoints

The receiver provides these HTTP endpoints:

- **GET `/api/sensor-data`** — Returns current sensor readings as JSON
  ```json
  {
    "temperature": 25.3,
    "pressure": 1013.25,
    "humidity": 45.2,
    "gasResistance": 5000,
    "magnetic": {
      "x": 12.3,
      "y": -5.6,
      "z": 18.9
    },
    "lastUpdate": 12345678
  }
  ```

- **GET `/api/status`** — Returns receiver status
  ```json
  {
    "wifiConnected": true,
    "ip": "192.168.1.100",
    "uptime": 123456789
  }
  ```

## Data Flow
1. Sender nodes transmit sensor data via LoRa
2. Receiver ESP32-S3 receives data and parses it
3. Dashboard polls `/api/sensor-data` every 2 seconds
4. Dashboard updates the display with live values

