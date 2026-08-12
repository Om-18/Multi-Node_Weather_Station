#include <Arduino.h>
#include <ArduinoJson.h>

// =====================================================
// LoRa UART CONFIGURATION
// =====================================================

HardwareSerial lora(1);

#define LORA_RX 16
#define LORA_TX 17

#define LORA_BAUD 115200
#define USB_BAUD 115200

// RYLR896 receiver configuration
const int LORA_ADDRESS = 2;
const int LORA_NETWORK_ID = 10;

// =====================================================
// SENSOR DATA
// =====================================================

struct SensorData {

  int nodeId = 0;

  float temperature = 0.0;
  float pressure = 0.0;
  float humidity = 0.0;
  float gasResistance = 0.0;

  float magX = 0.0;
  float magY = 0.0;
  float magZ = 0.0;

  int rssi = 0;
  int snr = 0;

  unsigned long timestamp = 0;
  bool dataReceived = false;
};

SensorData sensorData;

// =====================================================
// LoRa RECEIVE BUFFER
// =====================================================

String loraBuf = "";


// =====================================================
// SETUP
// =====================================================

void setup() {

  // ---------------------------------------------------
  // USB UART → Laptop
  // ---------------------------------------------------

  Serial.begin(USB_BAUD);

  // ---------------------------------------------------
  // UART → RYLR896
  // GPIO 16 = RX
  // GPIO 17 = TX
  // ---------------------------------------------------

  lora.begin(
    LORA_BAUD,
    SERIAL_8N1,
    LORA_RX,
    LORA_TX
  );

  delay(2000);

  // ---------------------------------------------------
  // Configure RYLR896
  // ---------------------------------------------------

  lora.println("AT+ADDRESS=2");
  delay(500);

  lora.println("AT+NETWORKID=10");
  delay(500);

  // ---------------------------------------------------
  // IMPORTANT:
  // Do NOT print debugging information to Serial.
  // Serial is the JSON communication channel.
  // ---------------------------------------------------
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // ---------------------------------------------------
  // Read LoRa UART without blocking
  // ---------------------------------------------------

  while (lora.available()) {

    char c = (char)lora.read();

    // Complete RYLR896 line
    if (c == '\n') {

      loraBuf.trim();

      if (loraBuf.length() > 0) {
        processLoRaMessage(loraBuf);
      }

      loraBuf = "";
    }

    // Ignore CR
    else if (c != '\r') {

      loraBuf += c;

      // Safety against corrupted input
      if (loraBuf.length() > 512) {

        loraBuf = "";
      }
    }
  }
}


// =====================================================
// PROCESS COMPLETE LORA MESSAGE
// =====================================================

void processLoRaMessage(String msg) {

  // ---------------------------------------------------
  // Expected RYLR896 format:
  //
  // +RCV=1,45,A:T|T:25.5|P:1012.4|H:60.2|G:12345|M:1.2,2.3,3.4,-80,9
  //
  // +RCV=
  // ADDRESS
  // LENGTH
  // DATA
  // RSSI
  // SNR
  // ---------------------------------------------------

  if (msg.indexOf("+RCV=") == -1) {

    // Ignore +OK, +READY, etc.
    return;
  }

  int rcvStart = msg.indexOf("+RCV=");

  String rcvMsg = msg.substring(rcvStart + 5);


  // ===================================================
  // Extract ADDRESS
  // ===================================================

  int firstComma = rcvMsg.indexOf(',');

  if (firstComma == -1) {
    return;
  }

  String addrStr =
    rcvMsg.substring(0, firstComma);

  sensorData.nodeId = addrStr.toInt();


  // ===================================================
  // Extract LENGTH
  // ===================================================

  int secondComma =
    rcvMsg.indexOf(',', firstComma + 1);

  if (secondComma == -1) {
    return;
  }


  // ===================================================
  // Everything after LENGTH
  //
  // DATA,RSSI,SNR
  // ===================================================

  String remainder =
    rcvMsg.substring(secondComma + 1);


  // ===================================================
  // Extract SNR
  // ===================================================

  int lastComma =
    remainder.lastIndexOf(',');

  if (lastComma == -1) {
    return;
  }

  String snrStr =
    remainder.substring(lastComma + 1);


  // ===================================================
  // Extract RSSI
  // ===================================================

  String beforeSnr =
    remainder.substring(0, lastComma);

  int secondLastComma =
    beforeSnr.lastIndexOf(',');

  if (secondLastComma == -1) {
    return;
  }

  String rssiStr =
    beforeSnr.substring(secondLastComma + 1);


  // ===================================================
  // Extract actual sensor payload
  // ===================================================

  String payload =
    beforeSnr.substring(0, secondLastComma);


  sensorData.rssi = rssiStr.toInt();
  sensorData.snr = snrStr.toInt();


  // ===================================================
  // Parse sensor payload
  // ===================================================

  parseSensorData(payload);


  // ===================================================
  // Send JSON to laptop
  // ===================================================

  sendJSON();
}


// =====================================================
// PARSE SENSOR PAYLOAD
// =====================================================
//
// Expected:
//
// A:Ax,Ay,Az
// T:temperature
// P:pressure
// H:humidity
// G:gasResistance
// M:magX,magY,magZ
//
// Example:
//
// A:1,2,3|T:25.4|P:1012.3|H:55.2|G:12456|M:1.2,2.3,3.4
//
// =====================================================

void parseSensorData(String data) {

  // ---------------------------------------------------
  // Temperature
  // ---------------------------------------------------

  int tStart = data.indexOf("T:");

  if (tStart != -1) {

    int tEnd =
      data.indexOf("|", tStart);

    if (tEnd == -1) {
      tEnd = data.length();
    }

    sensorData.temperature =
      data.substring(
        tStart + 2,
        tEnd
      ).toFloat();
  }


  // ---------------------------------------------------
  // Pressure
  // ---------------------------------------------------

  int pStart = data.indexOf("P:");

  if (pStart != -1) {

    int pEnd =
      data.indexOf("|", pStart);

    if (pEnd == -1) {
      pEnd = data.length();
    }

    sensorData.pressure =
      data.substring(
        pStart + 2,
        pEnd
      ).toFloat();
  }


  // ---------------------------------------------------
  // Humidity
  // ---------------------------------------------------

  int hStart = data.indexOf("H:");

  if (hStart != -1) {

    int hEnd =
      data.indexOf("|", hStart);

    if (hEnd == -1) {
      hEnd = data.length();
    }

    sensorData.humidity =
      data.substring(
        hStart + 2,
        hEnd
      ).toFloat();
  }


  // ---------------------------------------------------
  // Gas Resistance
  // ---------------------------------------------------

  int gStart = data.indexOf("G:");

  if (gStart != -1) {

    int gEnd =
      data.indexOf("|", gStart);

    if (gEnd == -1) {
      gEnd = data.length();
    }

    sensorData.gasResistance =
      data.substring(
        gStart + 2,
        gEnd
      ).toFloat();
  }


  // ---------------------------------------------------
  // Magnetic Field
  // ---------------------------------------------------

  int mStart = data.indexOf("M:");

  if (mStart != -1) {

    int mEnd =
      data.indexOf("|", mStart);

    if (mEnd == -1) {
      mEnd = data.length();
    }

    String magStr =
      data.substring(
        mStart + 2,
        mEnd
      );


    int comma1 =
      magStr.indexOf(",");

    int comma2 =
      magStr.indexOf(
        ",",
        comma1 + 1
      );


    if (comma1 != -1 && comma2 != -1) {

      sensorData.magX =
        magStr.substring(
          0,
          comma1
        ).toFloat();

      sensorData.magY =
        magStr.substring(
          comma1 + 1,
          comma2
        ).toFloat();

      sensorData.magZ =
        magStr.substring(
          comma2 + 1
        ).toFloat();
    }
  }


  // ---------------------------------------------------
  // Timestamp
  // ---------------------------------------------------

  sensorData.timestamp = millis();

  sensorData.dataReceived = true;
}


// =====================================================
// SEND JSON TO LAPTOP
// =====================================================

void sendJSON() {

  StaticJsonDocument<512> doc;


  // ---------------------------------------------------
  // Basic information
  // ---------------------------------------------------

  doc["nodeId"] =
    sensorData.nodeId;

  // ---------------------------------------------------
  // Sensor data
  // ---------------------------------------------------

  doc["temperature"] =
    sensorData.temperature;

  doc["pressure"] =
    sensorData.pressure;

  doc["humidity"] =
    sensorData.humidity;

  doc["gasResistance"] =
    sensorData.gasResistance;


  // ---------------------------------------------------
  // Magnetic sensor
  // ---------------------------------------------------

  doc["magnetic"]["x"] =
    sensorData.magX;

  doc["magnetic"]["y"] =
    sensorData.magY;

  doc["magnetic"]["z"] =
    sensorData.magZ;


  // ---------------------------------------------------
  // LoRa link information
  // ---------------------------------------------------

  doc["rssi"] =
    sensorData.rssi;

  doc["snr"] =
    sensorData.snr;


  // ---------------------------------------------------
  // Timestamp
  // ---------------------------------------------------

  doc["timestamp"] =
    sensorData.timestamp;


  // ---------------------------------------------------
  // Status
  // ---------------------------------------------------

  doc["dataReceived"] =
    sensorData.dataReceived;


  // ---------------------------------------------------
  // IMPORTANT:
  // One JSON object = one line
  // ---------------------------------------------------

  serializeJson(doc, Serial);

  Serial.println();
}