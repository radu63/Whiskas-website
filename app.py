from flask import Flask, render_template, jsonify, request
import time
import threading

app = Flask(__name__)

bot_state = {
    "POS": {
        "left_wheel": 0,
        "right_wheel": 0,
        "distance": 0,
        "gripper": 0,
        "time": ""
    },
    "Wall-E": {
        "left_wheel": 0,
        "right_wheel": 0,
        "distance": 0,
        "gripper": 0,
        "time": ""
    },
    "Dazey": {
        "left_wheel": 0,
        "right_wheel": 0,
        "distance": 0,
        "gripper": 0,
        "time": ""
    }
}

last_update_time = 0

arduino_connected = False

HEADER_MAP = {

    # POS
    129: ("POS", "left_wheel"),
    130: ("POS", "right_wheel"),
    131: ("POS", "gripper"),
    133: ("POS", "distance"),

    # Wall-E
    65:  ("Wall-E", "left_wheel"),
    66:  ("Wall-E", "right_wheel"),
    67:  ("Wall-E", "gripper"),
    69:  ("Wall-E", "distance"),

    # Dazey
    33:  ("Dazey", "left_wheel"),
    34:  ("Dazey", "right_wheel"),
    35:  ("Dazey", "gripper"),
    37:  ("Dazey", "distance"),
}

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/update", methods=["POST"])
def update():
    global arduino_connected
    global last_update_time

    data = request.json
    bot = data["bot"]
    key = data["key"]
    val = data["val"]

    bot_state[bot][key] = val
    arduino_connected = True
    last_update_time = time.time()

    return "OK"

@app.route("/status")
def status():
    global arduino_connected

    if last_update_time != 0:
        if time.time() - last_update_time > 3:
            arduino_connected = False

    return jsonify({
        "connected": arduino_connected,
        "robots": bot_state
    })

def update_time():
    while True:
        now = time.strftime("%H:%M:%S")
        for bot in bot_state:
            bot_state[bot]["time"] = now
        time.sleep(0.5)

threading.Thread(target=update_time, daemon=True).start()

if __name__ == "__main__":
    import os
    port = int(os.environ.get("PORT", 10000))
    app.run(host="0.0.0.0", port=port)