import serial
import json
import time


# ==============================
# Configuration
# ==============================

SERIAL_PORT = "COM3"       # Change this
BAUD_RATE = 115200


# ==============================
# Connect to ESP32
# ==============================

print(f"Connecting to {SERIAL_PORT}...")

ser = serial.Serial(
    port=SERIAL_PORT,
    baudrate=BAUD_RATE,
    timeout=1
)

# Give ESP32 time to reset after serial connection
time.sleep(2)

print("Connected!")
print("Waiting for JSON data...\n")


# ==============================
# Receive data
# ==============================

while True:

    try:

        # Read one complete line
        line = ser.readline().decode("utf-8", errors="ignore").strip()

        if not line:
            continue

        print("RAW:", line)

        # Try to parse JSON
        try:
            data = json.loads(line)

        except json.JSONDecodeError:
            print("Not JSON - ignored\n")
            continue

        # ==============================
        # Parsed data
        # ==============================

        print("Parsed JSON:")

        print("  Node ID     :", data.get("nodeId"))
        print("  Temperature :", data.get("temperature"))
        print("  Pressure    :", data.get("pressure"))
        print("  Humidity    :", data.get("humidity"))

        magnetic = data.get("magnetic", {})

        print("  Magnetic X  :", magnetic.get("x"))
        print("  Magnetic Y  :", magnetic.get("y"))
        print("  Magnetic Z  :", magnetic.get("z"))

        print("  RSSI        :", data.get("rssi"))
        print("  SNR         :", data.get("snr"))

        print("--------------------------------")

    except KeyboardInterrupt:

        print("\nStopping...")

        break

    except serial.SerialException as e:

        print("Serial error:", e)

        break


ser.close()