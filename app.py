from flask import Flask, render_template, jsonify
import threading
import time
import serial
####################
import os
ON_RENDER = os.environ.get("RENDER") == "true"
####################
app = Flask(__name__)

# --- CHANGE THIS to your Arduino port ---
ARDUINO_PORT = "/dev/tty.usbmodem48CA435A9B082"
BAUD_RATE = 9600

# --- Robot state ---
bot_state = {
    "POS":    {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "time": None},
    "Wall-E": {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "time": None},
    "Dazey":  {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "time": None}
}

# --- Header mapping from Arduino defines ---
HEADER_MAP = {
    0b00100001: ("Dazey", "left_wheel"),
    0b00100010: ("Dazey", "right_wheel"),
    0b00100011: ("Dazey", "gripper"),
    0b00100100: ("Dazey", "linestate"),
    0b00100101: ("Dazey", "distance"),  # sonar
    0b00101111: ("Dazey", "done"),

    0b10000001: ("POS", "left_wheel"),
    0b10000010: ("POS", "right_wheel"),
    0b10000011: ("POS", "gripper"),
    0b10000100: ("POS", "linestate"),
    0b10000101: ("POS", "distance"),    # sonar
    0b10001111: ("POS", "done"),

    0b01000001: ("Wall-E", "left_wheel"),
    0b01000010: ("Wall-E", "right_wheel"),
    0b01000011: ("Wall-E", "gripper"),
    0b01000100: ("Wall-E", "linestate"),
    0b01000101: ("Wall-E", "distance"),  # sonar
    0b01001111: ("Wall-E", "done"),
}
HEADERS = {
    0b00100001,
    0b00100010,
    0b00100011,
    0b00100100,
    0b00100101,  # sonar
    0b00101111,

    0b10000001,
    0b10000010,
    0b10000011,
    0b10000100,
    0b10000101,    # sonar
    0b10001111,

    0b01000001,
    0b01000010,
    0b01000011,
    0b01000100,
    0b01000101,  # sonar
    0b01001111
}

# --- Serial setup ---
ser = None
arduino_connected = False

try:
    ser = serial.Serial(ARDUINO_PORT, BAUD_RATE, timeout=0.05)
    arduino_connected = True
    print("✅ Arduino connected")
except:
    print("⚠️ Arduino not connected — dashboard will run with empty data")

# --- Background update loop ---
def update_loop():
    while True:
        now = time.strftime("%H:%M:%S")

        if arduino_connected and ser.in_waiting >= 2:
            peek_header: bytearray
            peek_header = [0, 0]  # read first byte
            thing = ser.readinto(peek_header)
            # peek_header[0] = int.from_bytes(peek_header[0], "big")
            # peek_header[1] = int.from_bytes(peek_header[1], "big")
            if peek_header[1] in HEADERS:
                # value = ser.read(1)  # read second byte
                bot_name, key = HEADER_MAP[peek_header[1]]
                print("worked")
                if key == "done":
                    print(f"{bot_name} is done!")
                else:
                    bot_state[bot_name][key] = peek_header[0]
            else:
                # Not a valid header, skip one byte
                print(f"Skipped unknown byte: {thing}")
                

        # Always update last updated time
        for r in bot_state:
            bot_state[r]["time"] = now

        time.sleep(0.05)  # 20 FPS

# Start background thread
threading.Thread(target=update_loop, daemon=True).start()

# --- Flask routes ---
########################################
@app.route("/")
def index():
    return "Dashboard backend is running"
########################################
@app.route("/status")
def status():
    return jsonify({
        "connected": arduino_connected,
        "robots": bot_state
    })

if __name__ == "__main__":
    # Run on localhost:8000
    app.run(debug=True, host="0.0.0.0", port=8000)
