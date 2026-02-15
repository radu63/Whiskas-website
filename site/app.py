from flask import Flask, render_template, jsonify
import threading
import time
import serial
import os

ON_RENDER = os.environ.get("RENDER") == "true"

app = Flask(__name__)

ARDUINO_PORT = "/dev/tty.usbmodem48CA435A9B082"
BAUD_RATE = 9600

bot_state = {
    "POS":    {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "linestate": None, "time": None},
    "Wall-E": {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "linestate": None, "time": None},
    "Dazey":  {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "linestate": None, "time": None}
}

HEADER_MAP = {
    0b00100001: ("Dazey", "left_wheel"),
    0b00100010: ("Dazey", "right_wheel"),
    0b00100011: ("Dazey", "gripper"),
    0b00100100: ("Dazey", "linestate"),
    0b00100101: ("Dazey", "distance"),
    0b00101111: ("Dazey", "done"),

    0b10000001: ("POS", "left_wheel"),
    0b10000010: ("POS", "right_wheel"),
    0b10000011: ("POS", "gripper"),
    0b10000100: ("POS", "linestate"),
    0b10000101: ("POS", "distance"),
    0b10001111: ("POS", "done"),

    0b01000001: ("Wall-E", "left_wheel"),
    0b01000010: ("Wall-E", "right_wheel"),
    0b01000011: ("Wall-E", "gripper"),
    0b01000100: ("Wall-E", "linestate"),
    0b01000101: ("Wall-E", "distance"),
    0b01001111: ("Wall-E", "done"),
}

HEADERS = set(HEADER_MAP.keys())

# Serial setup
ser = None
arduino_connected = False

try:
    ser = serial.Serial(ARDUINO_PORT, BAUD_RATE, timeout=0.05)
    arduino_connected = True
    print("Arduino connected")
except:
    print("Arduino not connected — dashboard will run with empty data")

# Background update loop
def update_loop():
    while True:
        now = time.strftime("%H:%M:%S")

        if arduino_connected and ser.in_waiting >= 2:
            raw = ser.read(2)

            header = raw[0]
            value  = raw[1]

            if header in HEADERS:
                bot_name, key = HEADER_MAP[header]

                if key == "done":
                    print(f"{bot_name} is done!")
                else:
                    bot_state[bot_name][key] = value
                    print(f"{bot_name} {key} = {value}")
            else:
                print(f"Skipped unknown header: {header}, value: {value}")

        for r in bot_state:
            bot_state[r]["time"] = now

        time.sleep(0.05)

threading.Thread(target=update_loop, daemon=True).start()

# Flask routes
@app.route("/")
def index():
    return render_template("index.html")


@app.route("/status")
def status():
    return jsonify({
        "connected": arduino_connected,
        "robots": bot_state
    })

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=8000)
