#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_wifi.h>

HardwareSerial lora(1);
WebServer server(80);

// WiFi credentials
const char *ssid = "RG17";
const char *password = "Starlord";

// Latest sensor data
struct SensorData {
  float temperature = 0.0;
  float pressure = 0.0;
  float humidity = 0.0;
  float gasResistance = 0.0;
  float magX = 0.0, magY = 0.0, magZ = 0.0;
  int rssi = 0;
  int snr = 0;
  int nodeId = 0;
  unsigned long lastUpdate = 0;
  bool dataReceived = false;
} sensorData;

void setup() {
  Serial.begin(115200);
  lora.begin(115200, SERIAL_8N1, 18, 17);

  delay(2000);

  lora.println("AT+ADDRESS=2");
  delay(500);
  lora.println("AT+NETWORKID=10");
  delay(500);

  Serial.println("===== LORA RECEIVER =====");

  // WiFi setup
  connectToWiFi();

  // Handle CORS preflight
  server.on("/api/sensor-data", HTTP_OPTIONS, handleCORS);
  server.on("/api/status", HTTP_OPTIONS, handleCORS);
  server.on("/api/health", HTTP_OPTIONS, handleCORS);

  // HTTP endpoints
  server.on("/api/sensor-data", HTTP_GET, handleSensorData);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/health", HTTP_GET, handleHealth);
  server.begin();

  Serial.println("HTTP server started");
}

// ── Non-blocking serial receive buffer ──
String loraBuf = "";
unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 15000; // check WiFi every 15 s

void loop() {
  // ── 1. Always service HTTP clients first (never starve this) ──
  server.handleClient();

  // ── 2. Non-blocking LoRa read: one char at a time ──
  while (lora.available()) {
    char c = (char)lora.read();

    if (c == '\n') {
      // We have a complete line — process it
      loraBuf.trim();
      if (loraBuf.length() > 0) {
        processLoRaMessage(loraBuf);
      }
      loraBuf = "";
    } else if (c != '\r') {
      loraBuf += c;
      // Safety: prevent runaway buffer
      if (loraBuf.length() > 512) {
        Serial.println("WARN: LoRa buffer overflow, flushing");
        loraBuf = "";
      }
    }
  }

  // ── 3. WiFi watchdog — auto-reconnect if WiFi drops ──
  if (millis() - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi lost! Reconnecting...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      int tries = 0;
      while (WiFi.status() != WL_CONNECTED && tries < 20) {
        delay(250);
        tries++;
        // Keep servicing HTTP during reconnect attempts
        server.handleClient();
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi reconnected! IP: " + WiFi.localIP().toString());
      } else {
        Serial.println("WiFi reconnect failed, will retry...");
      }
    }
  }
}

// ── Process a complete LoRa message line ──
void processLoRaMessage(String msg) {
  Serial.println("RAW: " + msg);

  // Parse +RCV message: +RCV=<ADDR>,<LEN>,<DATA>,<RSSI>,<SNR>
  if (msg.indexOf("+RCV=") == -1) {
    Serial.println("(non-RCV line, skipping)");
    return;
  }

  int rcvStart = msg.indexOf("+RCV=");
  String rcvMsg = msg.substring(rcvStart + 5); // skip "+RCV="

  // Split: ADDR,LEN,DATA...,RSSI,SNR
  int firstComma = rcvMsg.indexOf(',');
  if (firstComma == -1) { Serial.println("Bad format"); return; }

  String addrStr = rcvMsg.substring(0, firstComma);
  sensorData.nodeId = addrStr.toInt();

  int secondComma = rcvMsg.indexOf(',', firstComma + 1);
  if (secondComma == -1) { Serial.println("Bad format"); return; }

  // Everything after second comma: DATA,RSSI,SNR
  String remainder = rcvMsg.substring(secondComma + 1);

  // RSSI and SNR are the last two comma-separated values
  int lastComma = remainder.lastIndexOf(',');
  if (lastComma == -1) { Serial.println("Bad format"); return; }

  String snrStr = remainder.substring(lastComma + 1);
  String beforeSnr = remainder.substring(0, lastComma);

  int secondLastComma = beforeSnr.lastIndexOf(',');
  if (secondLastComma == -1) { Serial.println("Bad format"); return; }

  String rssiStr = beforeSnr.substring(secondLastComma + 1);
  String payload = beforeSnr.substring(0, secondLastComma);

  sensorData.rssi = rssiStr.toInt();
  sensorData.snr = snrStr.toInt();

  Serial.println("Node ID: " + addrStr);
  Serial.println("Payload: " + payload);
  Serial.println("RSSI: " + rssiStr + " SNR: " + snrStr);

  parseSensorData(payload);
  Serial.println("------------------------");
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);                     // ★ Disable modem sleep — keeps radio always on
  WiFi.setAutoReconnect(true);              // ★ Auto-reconnect at driver level
  WiFi.begin(ssid, password);

  // Wait for connection with retry and fallback to reset if not connected
  int attempts = 0;
  const int maxAttempts = 30;
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi after multiple attempts. "
                   "Restarting ESP...");
    delay(2000);
    ESP.restart();
  } else {
    // ★ Force WiFi power saving OFF at the ESP-IDF level
    esp_wifi_set_ps(WIFI_PS_NONE);
    Serial.println("\nWiFi connected! (sleep DISABLED)");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void parseSensorData(String data) {
  // Expected format: A:Ax,Ay,Az|T:temp|P:pressure|H:humidity|G:gas|M:mx,my,mz

  int tStart = data.indexOf("T:");
  int pStart = data.indexOf("P:");
  int hStart = data.indexOf("H:");
  int gStart = data.indexOf("G:");
  int mStart = data.indexOf("M:");

  if (tStart != -1) {
    int tEnd = data.indexOf("|", tStart);
    if (tEnd == -1) tEnd = data.length();
    sensorData.temperature = data.substring(tStart + 2, tEnd).toFloat();
  }

  if (pStart != -1) {
    int pEnd = data.indexOf("|", pStart);
    if (pEnd == -1) pEnd = data.length();
    sensorData.pressure = data.substring(pStart + 2, pEnd).toFloat();
  }

  if (hStart != -1) {
    int hEnd = data.indexOf("|", hStart);
    if (hEnd == -1) hEnd = data.length();
    sensorData.humidity = data.substring(hStart + 2, hEnd).toFloat();
  }

  if (gStart != -1) {
    int gEnd = data.indexOf("|", gStart);
    if (gEnd == -1) gEnd = data.length();
    sensorData.gasResistance = data.substring(gStart + 2, gEnd).toFloat();
  }

  if (mStart != -1) {
    int mEnd = data.indexOf("|", mStart);
    if (mEnd == -1) mEnd = data.length();
    String magStr = data.substring(mStart + 2, mEnd);
    int comma1 = magStr.indexOf(",");
    int comma2 = magStr.indexOf(",", comma1 + 1);
    if (comma1 != -1 && comma2 != -1) {
      sensorData.magX = magStr.substring(0, comma1).toFloat();
      sensorData.magY = magStr.substring(comma1 + 1, comma2).toFloat();
      sensorData.magZ = magStr.substring(comma2 + 1).toFloat();
    }
  }

  sensorData.lastUpdate = millis();
  sensorData.dataReceived = true;

  Serial.println(">> Data parsed successfully!");
  Serial.println("   T=" + String(sensorData.temperature) +
                 " P=" + String(sensorData.pressure) +
                 " H=" + String(sensorData.humidity) +
                 " G=" + String(sensorData.gasResistance));
}

void addCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Connection", "close");  // ★ Prevent TCP socket exhaustion
  server.sendHeader("Cache-Control", "no-cache, no-store");
}

void handleCORS() {
  addCORSHeaders();
  server.send(204);
}

void handleSensorData() {
  addCORSHeaders();

  DynamicJsonDocument doc(384);
  doc["temperature"] = sensorData.temperature;
  doc["pressure"] = sensorData.pressure;
  doc["humidity"] = sensorData.humidity;
  doc["gasResistance"] = sensorData.gasResistance;

  doc["magnetic"]["x"] = sensorData.magX;
  doc["magnetic"]["y"] = sensorData.magY;
  doc["magnetic"]["z"] = sensorData.magZ;

  doc["rssi"] = sensorData.rssi;
  doc["snr"] = sensorData.snr;
  doc["nodeId"] = sensorData.nodeId;
  doc["lastUpdate"] = sensorData.lastUpdate;
  doc["dataReceived"] = sensorData.dataReceived;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleStatus() {
  addCORSHeaders();

  DynamicJsonDocument doc(256);
  doc["wifiConnected"] = (WiFi.status() == WL_CONNECTED);
  doc["ip"] = WiFi.localIP().toString();
  doc["uptime"] = millis();
  doc["dataReceived"] = sensorData.dataReceived;
  doc["lastUpdate"] = sensorData.lastUpdate;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleHealth() {
  addCORSHeaders();

  DynamicJsonDocument doc(64);
  doc["status"] = "ok";

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}