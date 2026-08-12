#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <Adafruit_DPS310.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Tlv493d.h>


// =====================================================
// SENSOR OBJECTS
// =====================================================

Adafruit_DPS310 dps;
Adafruit_BME680 bme;
Tlv493d magSensor;


// =====================================================
// RECEIVER MAC ADDRESS
// =====================================================

uint8_t receiverAddress[] = {
  0x1C, 0xC3, 0xAB,
  0xB3, 0xE8, 0x18
};


// =====================================================
// NODE CONFIGURATION
// =====================================================

const uint8_t NODE_ID = 1;


// =====================================================
// DATA PACKET
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
// GLOBAL VARIABLES
// =====================================================

SensorPacket packet;

uint32_t sequenceNumber = 0;


// =====================================================
// ESP-NOW SEND CALLBACK
// =====================================================

void OnDataSent(
  const wifi_tx_info_t *info,
  esp_now_send_status_t status
) {

  Serial.print("ESP-NOW Send Status: ");

  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("SUCCESS");
  }
  else {
    Serial.println("FAILED");
  }
}


// =====================================================
// PRINT MAC ADDRESS
// =====================================================

void printMacAddress() {

  uint8_t mac[6];

  esp_wifi_get_mac(WIFI_IF_STA, mac);

  Serial.printf(
    "Sender MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
    mac[0],
    mac[1],
    mac[2],
    mac[3],
    mac[4],
    mac[5]
  );
}


// =====================================================
// INITIALIZE ESP-NOW
// =====================================================

bool initializeESPNow() {

  if (esp_now_init() != ESP_OK) {

    Serial.println("ERROR: ESP-NOW initialization failed!");

    return false;
  }

  Serial.println("ESP-NOW initialized successfully");


  // Register send callback

  esp_now_register_send_cb(OnDataSent);


  // Configure receiver

  esp_now_peer_info_t peerInfo = {};

  memcpy(
    peerInfo.peer_addr,
    receiverAddress,
    6
  );

  // Use current Wi-Fi channel

  peerInfo.channel = 0;

  // No encryption for prototype

  peerInfo.encrypt = false;


  // Add receiver

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {

    Serial.println("ERROR: Failed to add receiver peer!");

    return false;
  }

  Serial.println("Receiver peer added successfully");

  return true;
}


// =====================================================
// INITIALIZE SENSORS
// =====================================================

bool initializeSensors() {

  bool success = true;


  // ---------------------------------------------------
  // DPS310
  // ---------------------------------------------------

  Serial.println("Initializing DPS310...");

  if (!dps.begin_I2C()) {

    Serial.println("ERROR: DPS310 not found!");

    success = false;
  }
  else {

    Serial.println("DPS310 initialized");

    // Optional configuration

    dps.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
    dps.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);
  }


  // ---------------------------------------------------
  // BME680
  // ---------------------------------------------------

  Serial.println("Initializing BME680...");

  if (!bme.begin(0x76)) {

    Serial.println("ERROR: BME680 not found!");

    success = false;
  }
  else {

    Serial.println("BME680 initialized");


    // Temperature oversampling

    bme.setTemperatureOversampling(
      BME680_OS_8X
    );


    // Humidity oversampling

    bme.setHumidityOversampling(
      BME680_OS_2X
    );


    // Pressure oversampling

    bme.setPressureOversampling(
      BME680_OS_4X
    );


    // IIR filter

    bme.setIIRFilterSize(
      BME680_FILTER_SIZE_3
    );


    // Gas heater

    bme.setGasHeater(
      320,
      150
    );
  }


  // ---------------------------------------------------
  // TLV493D
  // ---------------------------------------------------

  Serial.println("Initializing TLV493D...");

  magSensor.begin();

  Serial.println("TLV493D initialized");


  return success;
}


// =====================================================
// READ SENSORS
// =====================================================

bool readSensors() {


  // ---------------------------------------------------
  // DPS310
  // ---------------------------------------------------

  sensors_event_t temp_event;
  sensors_event_t pressure_event;

  bool dpsDataAvailable = false;


  if (
    dps.temperatureAvailable() &&
    dps.pressureAvailable()
  ) {

    dps.getEvents(
      &temp_event,
      &pressure_event
    );

    dpsDataAvailable = true;
  }


  // ---------------------------------------------------
  // BME680
  // ---------------------------------------------------

  bool bmeDataAvailable = bme.performReading();


  // ---------------------------------------------------
  // TLV493D
  // ---------------------------------------------------

  magSensor.updateData();

  float mx = magSensor.getX();
  float my = magSensor.getY();
  float mz = magSensor.getZ();


  // ---------------------------------------------------
  // Check sensor availability
  // ---------------------------------------------------

  if (!dpsDataAvailable) {

    Serial.println("WARNING: DPS310 data unavailable");
  }

  if (!bmeDataAvailable) {

    Serial.println("WARNING: BME680 reading failed");
  }


  // ---------------------------------------------------
  // Fill packet
  // ---------------------------------------------------

  packet.nodeId = NODE_ID;

  packet.sequence = sequenceNumber++;

  packet.temperature = temp_event.temperature;

  packet.pressure = pressure_event.pressure;

  packet.humidity = bme.humidity;

  packet.gasResistance = bme.gas_resistance;

  packet.magX = mx;
  packet.magY = my;
  packet.magZ = mz;


  return (
    dpsDataAvailable &&
    bmeDataAvailable
  );
}


// =====================================================
// PRINT PACKET
// =====================================================

void printPacket() {

  Serial.println();
  Serial.println("========== SENSOR DATA ==========");

  Serial.print("Node ID       : ");
  Serial.println(packet.nodeId);

  Serial.print("Sequence      : ");
  Serial.println(packet.sequence);

  Serial.print("Temperature   : ");
  Serial.print(packet.temperature);
  Serial.println(" °C");

  Serial.print("Pressure      : ");
  Serial.print(packet.pressure);
  Serial.println(" hPa");

  Serial.print("Humidity      : ");
  Serial.print(packet.humidity);
  Serial.println(" %");

  Serial.print("Gas Resistance: ");
  Serial.print(packet.gasResistance);
  Serial.println(" Ohms");

  Serial.print("Magnetic X    : ");
  Serial.println(packet.magX);

  Serial.print("Magnetic Y    : ");
  Serial.println(packet.magY);

  Serial.print("Magnetic Z    : ");
  Serial.println(packet.magZ);

  Serial.println("=================================");
}


// =====================================================
// SEND SENSOR PACKET
// =====================================================

void sendSensorPacket() {

  esp_err_t result = esp_now_send(
    receiverAddress,
    (uint8_t *)&packet,
    sizeof(packet)
  );


  if (result == ESP_OK) {

    Serial.println("Sensor packet queued successfully");

  }
  else {

    Serial.printf(
      "ESP-NOW send error: 0x%X\n",
      result
    );
  }
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
  Serial.println("     ESP-NOW SENSOR SENDER");
  Serial.println("==========================================");


  // ---------------------------------------------------
  // I2C
  // ---------------------------------------------------

  Wire.begin(21, 22);

  Wire.setClock(100000);

  Serial.println("I2C initialized");
  Serial.println("SDA = GPIO21");
  Serial.println("SCL = GPIO22");


  // ---------------------------------------------------
  // Wi-Fi
  // ---------------------------------------------------

  WiFi.mode(WIFI_STA);

  delay(500);

  printMacAddress();


  // ---------------------------------------------------
  // ESP-NOW
  // ---------------------------------------------------

  if (!initializeESPNow()) {

    Serial.println();
    Serial.println("ESP-NOW setup FAILED!");
    Serial.println("System halted.");

    while (true) {
      delay(1000);
    }
  }


  // ---------------------------------------------------
  // Sensors
  // ---------------------------------------------------

  if (!initializeSensors()) {

    Serial.println();
    Serial.println("WARNING: One or more sensors failed!");
    Serial.println("Continuing anyway...");
  }


  // ---------------------------------------------------
  // Startup complete
  // ---------------------------------------------------

  Serial.println();
  Serial.println("==========================================");
  Serial.println("      SYSTEM READY");
  Serial.println("==========================================");

  Serial.println("Node ID       : 1");
  Serial.println("Receiver      : 1C:C3:AB:B3:E8:18");
  Serial.println("Transmission  : ESP-NOW");
  Serial.println("MPU6050       : NOT USED");
  Serial.println("DPS310        : ENABLED");
  Serial.println("BME680        : ENABLED");
  Serial.println("TLV493D       : ENABLED");

  Serial.println("==========================================");
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // ---------------------------------------------------
  // Read sensors
  // ---------------------------------------------------

  bool sensorOK = readSensors();


  // ---------------------------------------------------
  // Print data
  // ---------------------------------------------------

  printPacket();


  // ---------------------------------------------------
  // Send data
  // ---------------------------------------------------

  sendSensorPacket();


  // ---------------------------------------------------
  // Wait 3 seconds
  // ---------------------------------------------------

  Serial.println();
  Serial.println("Waiting 3 seconds...");
  Serial.println();

  delay(3000);
}