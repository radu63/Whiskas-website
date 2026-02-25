from flask import Flask, render_template, jsonify, request
import time
import threading

app = Flask(__name__)

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
    data = request.json
    bot = data["bot"]
    key = data["key"]
    val = data["val"]

    bot_state[bot][key] = val
    return "OK"

@app.route("/status")
def status():
    return jsonify(bot_state)

def update_time():
    while True:
        now = time.strftime("%H:%M:%S")
        for bot in bot_state:
            bot_state[bot]["time"] = now
        time.sleep(0.5)

threading.Thread(target=update_time, daemon=True).start()

if __name__ == "__main__":
    app.run(port=8000)