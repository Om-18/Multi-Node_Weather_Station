#include <Wire.h>
#include <Adafruit_DPS310.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Tlv493d.h>

#define MPU_ADDR 0x68

Adafruit_DPS310 dps;
Adafruit_BME680 bme;
Tlv493d magSensor;

HardwareSerial lora(2);

void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin(21, 22);
  Wire.setClock(100000);

  // LoRa init
  lora.begin(115200, SERIAL_8N1, 16, 17);
  delay(2000);

  lora.println("AT+ADDRESS=1");
  delay(500);
  lora.println("AT+NETWORKID=10");
  delay(500);

  Serial.println("===== SENSOR + LORA SENDER =====");

  // MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  // DPS310
  dps.begin_I2C();

  // BME680
  bme.begin(0x76);

  // TLV493D
  magSensor.begin();
}

void loop() {

  // ===== MPU6050 =====
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  int16_t ax = Wire.read()<<8 | Wire.read();
  int16_t ay = Wire.read()<<8 | Wire.read();
  int16_t az = Wire.read()<<8 | Wire.read();
  Wire.read(); Wire.read();
  int16_t gx = Wire.read()<<8 | Wire.read();
  int16_t gy = Wire.read()<<8 | Wire.read();
  int16_t gz = Wire.read()<<8 | Wire.read();

  float Ax = ax / 16384.0;
  float Ay = ay / 16384.0;
  float Az = az / 16384.0;

  // ===== DPS310 =====
  sensors_event_t temp_event, pressure_event;
  if (dps.temperatureAvailable() && dps.pressureAvailable()) {
    dps.getEvents(&temp_event, &pressure_event);
  }

  // ===== BME680 =====
  bme.performReading();

  // ===== TLV493D =====
  magSensor.updateData();
  float mx = magSensor.getX();
  float my = magSensor.getY();
  float mz = magSensor.getZ();

  // ===== FORMAT DATA =====
  String data = "A:";
  data += String(Ax,2)+","+String(Ay,2)+","+String(Az,2);

  data += "|T:";
  data += String(temp_event.temperature,1);

  data += "|P:";
  data += String(pressure_event.pressure,1);

  data += "|H:";
  data += String(bme.humidity,1);

  data += "|G:";
  data += String(bme.gas_resistance,0);

  data += "|M:";
  data += String(mx,1)+","+String(my,1)+","+String(mz,1);

  // ===== SEND VIA LORA =====
  String cmd = "AT+SEND=2," + String(data.length()) + "," + data;

  lora.println(cmd);

  Serial.println("Sent:");
  Serial.println(data);

  delay(3000);
}