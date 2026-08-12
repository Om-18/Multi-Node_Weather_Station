#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <ArduinoJson.h>


// =====================================================
// RECEIVER CONFIGURATION
// =====================================================

const uint8_t NODE_ID = 1;


// =====================================================
// SENSOR PACKET
// MUST MATCH THE SENDER EXACTLY
// =====================================================

struct SensorPacket {

  uint8_t nodeId;

  uint32_t sequence;

  float temperature;
  float pressure;
  float humidity;
  float gasResistance;

  float magX;
  float magY;
  float magZ;
};


// =====================================================
// GLOBAL PACKET
// =====================================================

SensorPacket receivedPacket;


// =====================================================
// ESP-NOW RECEIVE CALLBACK
// =====================================================

void OnDataRecv(
  const esp_now_recv_info_t *info,
  const uint8_t *data,
  int len
) {

  // ===================================================
  // Check packet size
  // ===================================================

  if (len != sizeof(SensorPacket)) {

    // Do NOT print debug information to Serial.
    // Serial is being used for JSON communication
    // with the laptop.

    return;
  }


  // ===================================================
  // Copy received data
  // ===================================================

  memcpy(
    &receivedPacket,
    data,
    sizeof(SensorPacket)
  );


  // ===================================================
  // Create JSON document
  // ===================================================

  StaticJsonDocument<512> doc;


  // ===================================================
  // Basic information
  // ===================================================

  doc["nodeId"] = receivedPacket.nodeId;

  doc["sequence"] = receivedPacket.sequence;


  // ===================================================
  // DPS310
  // ===================================================

  doc["temperature"] =
    receivedPacket.temperature;

  doc["pressure"] =
    receivedPacket.pressure;


  // ===================================================
  // BME680
  // ===================================================

  doc["humidity"] =
    receivedPacket.humidity;

  doc["gasResistance"] =
    receivedPacket.gasResistance;


  // ===================================================
  // TLV493D
  // ===================================================

  doc["magnetic"]["x"] =
    receivedPacket.magX;

  doc["magnetic"]["y"] =
    receivedPacket.magY;

  doc["magnetic"]["z"] =
    receivedPacket.magZ;


  // ===================================================
  // Timestamp
  // ===================================================

  doc["timestamp"] = millis();


  // ===================================================
  // Send JSON to laptop
  // ===================================================

  serializeJson(
    doc,
    Serial
  );

  Serial.println();
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  // ===================================================
  // USB UART → Laptop
  // ===================================================

  Serial.begin(115200);

  delay(2000);


  // ===================================================
  // Initialize WiFi
  // Required for ESP-NOW
  // ===================================================

  WiFi.mode(WIFI_STA);

  delay(500);


  // ===================================================
  // Initialize ESP-NOW
  // ===================================================

  if (esp_now_init() != ESP_OK) {

    // Do not print error to Serial because the
    // laptop expects JSON.

    return;
  }


  // ===================================================
  // Register receive callback
  // ===================================================

  esp_now_register_recv_cb(
    OnDataRecv
  );
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // ESP-NOW packets are received asynchronously
  // through OnDataRecv().

}