import serial
import json
import time


# =====================================================
# CONFIGURATION
# =====================================================

SERIAL_PORT = "COM6"       # Change this to your ESP32 COM port
BAUD_RATE = 115200


# =====================================================
# CONNECT TO ESP32
# =====================================================

print(f"Connecting to {SERIAL_PORT}...")

try:

    ser = serial.Serial(
        port=SERIAL_PORT,
        baudrate=BAUD_RATE,
        timeout=1
    )

except serial.SerialException as e:

    print("Could not open serial port!")
    print("Error:", e)
    exit()


# Give ESP32 time to reset after serial connection
time.sleep(2)

print("Connected!")
print("Waiting for JSON data...\n")


# =====================================================
# RECEIVE DATA
# =====================================================

try:

    while True:

        # -------------------------------------------------
        # Read one complete JSON line
        # -------------------------------------------------

        line = ser.readline().decode(
            "utf-8",
            errors="ignore"
        ).strip()


        if not line:
            continue


        # -------------------------------------------------
        # Show raw data
        # -------------------------------------------------

        print("RAW:")
        print(line)


        # -------------------------------------------------
        # Parse JSON
        # -------------------------------------------------

        try:

            data = json.loads(line)

        except json.JSONDecodeError:

            print("Not JSON - ignored")
            print("--------------------------------")
            continue


        # =================================================
        # PARSED DATA
        # =================================================

        print("Parsed JSON:")


        # -------------------------------------------------
        # Basic information
        # -------------------------------------------------

        print(
            "  Node ID       :",
            data.get("nodeId")
        )

        print(
            "  Sequence      :",
            data.get("sequence")
        )


        # -------------------------------------------------
        # DPS310
        # -------------------------------------------------

        print(
            "  Temperature   :",
            data.get("temperature"),
            "°C"
        )

        print(
            "  Pressure      :",
            data.get("pressure"),
            "hPa"
        )


        # -------------------------------------------------
        # BME680
        # -------------------------------------------------

        print(
            "  Humidity      :",
            data.get("humidity"),
            "%"
        )

        print(
            "  Gas Resistance:",
            data.get("gasResistance"),
            "Ohms"
        )


        # -------------------------------------------------
        # TLV493D
        # -------------------------------------------------

        magnetic = data.get(
            "magnetic",
            {}
        )

        print(
            "  Magnetic X    :",
            magnetic.get("x")
        )

        print(
            "  Magnetic Y    :",
            magnetic.get("y")
        )

        print(
            "  Magnetic Z    :",
            magnetic.get("z")
        )


        # -------------------------------------------------
        # Timestamp
        # -------------------------------------------------

        print(
            "  ESP32 Time    :",
            data.get("timestamp"),
            "ms"
        )


        print("--------------------------------")


# =====================================================
# STOP PROGRAM
# =====================================================

except KeyboardInterrupt:

    print("\nStopping...")


except serial.SerialException as e:

    print("\nSerial error:", e)


finally:

    ser.close()

    print("Serial port closed.")