HardwareSerial lora(1);

void setup() {
  Serial.begin(115200);
  lora.begin(115200, SERIAL_8N1, 18, 17);

  delay(2000);

  lora.println("AT+ADDRESS=2");
  delay(500);
  lora.println("AT+NETWORKID=10");
  delay(500);

  Serial.println("===== LORA RECEIVER =====");
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