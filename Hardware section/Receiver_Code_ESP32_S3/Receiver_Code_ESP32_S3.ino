#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

HardwareSerial lora(1);
WebServer server(80);

// WiFi credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Latest sensor data
struct SensorData {
  float temperature = 0.0;
  float pressure = 0.0;
  float humidity = 0.0;
  float gasResistance = 0.0;
  float magX = 0.0, magY = 0.0, magZ = 0.0;
  unsigned long lastUpdate = 0;
} sensorData;

void setup() {
  Serial.begin(115200);
  lora.begin(115200, SERIAL_8N1, 17, 18);

  delay(2000);

  lora.println("AT+ADDRESS=2");
  delay(500);
  lora.println("AT+NETWORKID=10");
  delay(500);

  Serial.println("===== LORA RECEIVER =====");

  // WiFi setup
  connectToWiFi();

  // HTTP endpoints
  server.on("/api/sensor-data", HTTP_GET, handleSensorData);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.begin();

  Serial.println("HTTP server started");
}

void loop() {

  if (lora.available()) {
    String msg = lora.readString();

    Serial.println("RAW:");
    Serial.println(msg);

    // Extract actual data
    int start = msg.indexOf(",");
    start = msg.indexOf(",", start+1);
    start = msg.indexOf(",", start+1);

    if (start != -1) {
      String data = msg.substring(start+1);
      Serial.println("Parsed Data:");
      Serial.println(data);
    }

    Serial.println("------------------------");
  }
  Serial.println(" ================= No data Received ================= ");
  delay(1000);
}