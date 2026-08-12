#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>


// =====================================================
// SENSOR PACKET
// Must exactly match the sender
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
// RECEIVE CALLBACK
// =====================================================

void OnDataRecv(
  const esp_now_recv_info_t *info,
  const uint8_t *data,
  int len
) {

  Serial.println();
  Serial.println("==========================================");
  Serial.println("        ESP-NOW PACKET RECEIVED");
  Serial.println("==========================================");


  // ---------------------------------------------------
  // Check packet size
  // ---------------------------------------------------

  if (len != sizeof(SensorPacket)) {

    Serial.print("ERROR: Unexpected packet size: ");
    Serial.println(len);

    Serial.print("Expected: ");
    Serial.println(sizeof(SensorPacket));

    Serial.println("Packet discarded.");

    return;
  }


  // ---------------------------------------------------
  // Copy received data into structure
  // ---------------------------------------------------

  memcpy(
    &receivedPacket,
    data,
    sizeof(SensorPacket)
  );


  // ---------------------------------------------------
  // Print sender MAC
  // ---------------------------------------------------

  Serial.print("Sender MAC: ");

  for (int i = 0; i < 6; i++) {

    if (i > 0) {
      Serial.print(":");
    }

    Serial.printf(
      "%02X",
      info->src_addr[i]
    );
  }

  Serial.println();


  // ---------------------------------------------------
  // Print packet information
  // ---------------------------------------------------

  Serial.print("Packet Size : ");
  Serial.println(len);

  Serial.print("Node ID     : ");
  Serial.println(receivedPacket.nodeId);

  Serial.print("Sequence    : ");
  Serial.println(receivedPacket.sequence);


  // ---------------------------------------------------
  // Print DPS310 data
  // ---------------------------------------------------

  Serial.println();
  Serial.println("--- DPS310 ---");

  Serial.print("Temperature : ");
  Serial.print(receivedPacket.temperature, 2);
  Serial.println(" °C");

  Serial.print("Pressure    : ");
  Serial.print(receivedPacket.pressure, 2);
  Serial.println(" hPa");


  // ---------------------------------------------------
  // Print BME680 data
  // ---------------------------------------------------

  Serial.println();
  Serial.println("--- BME680 ---");

  Serial.print("Humidity    : ");
  Serial.print(receivedPacket.humidity, 2);
  Serial.println(" %");

  Serial.print("Gas         : ");
  Serial.print(receivedPacket.gasResistance, 0);
  Serial.println(" Ohms");


  // ---------------------------------------------------
  // Print TLV493D data
  // ---------------------------------------------------

  Serial.println();
  Serial.println("--- TLV493D ---");

  Serial.print("Mag X       : ");
  Serial.println(receivedPacket.magX, 2);

  Serial.print("Mag Y       : ");
  Serial.println(receivedPacket.magY, 2);

  Serial.print("Mag Z       : ");
  Serial.println(receivedPacket.magZ, 2);


  Serial.println();
  Serial.println("==========================================");
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(2000);


  Serial.println();
  Serial.println();
  Serial.println("==========================================");
  Serial.println("          ESP-NOW RECEIVER");
  Serial.println("==========================================");


  // ---------------------------------------------------
  // Initialize Wi-Fi
  // ---------------------------------------------------

  WiFi.mode(WIFI_STA);

  delay(500);


  // ---------------------------------------------------
  // Print receiver MAC
  // ---------------------------------------------------

  uint8_t mac[6];

  esp_wifi_get_mac(
    WIFI_IF_STA,
    mac
  );


  Serial.printf(
    "Receiver MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
    mac[0],
    mac[1],
    mac[2],
    mac[3],
    mac[4],
    mac[5]
  );


  // ---------------------------------------------------
  // Initialize ESP-NOW
  // ---------------------------------------------------

  if (esp_now_init() != ESP_OK) {

    Serial.println();
    Serial.println("ERROR: ESP-NOW initialization FAILED!");

    while (true) {
      delay(1000);
    }
  }


  Serial.println("ESP-NOW initialized successfully");


  // ---------------------------------------------------
  // Register receive callback
  // ---------------------------------------------------

  esp_now_register_recv_cb(
    OnDataRecv
  );


  Serial.println("Receive callback registered");

  Serial.println();
  Serial.println("==========================================");
  Serial.println("       ESP-NOW RECEIVER READY");
  Serial.println("==========================================");
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // Nothing required here.
  //
  // ESP-NOW packets are handled by
  // OnDataRecv() automatically.

}