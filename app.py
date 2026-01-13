from flask import Flask, jsonify, request
import time
import os

app = Flask(__name__)

# The data is now stored here in the cloud
bot_state = {
    "POS":     {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "time": None},
    "Wall-E": {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "time": None},
    "Dazey":  {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "time": None}
}

@app.route("/")
def index():
    return "Robot Dashboard is Live"

@app.route("/status")
def status():
    return jsonify({"robots": bot_state})

@app.route("/update", methods=["POST"])
def update():
    data = request.json
    bot = data.get("bot")
    key = data.get("key")
    val = data.get("val")
    if bot in bot_state:
        bot_state[bot][key] = val
        bot_state[bot]["time"] = time.strftime("%H:%M:%S")
    return "OK", 200

if __name__ == "__main__":
    # Render requires binding to 0.0.0.0 and the PORT env variable
    port = int(os.environ.get("PORT", 10000))
    app.run(host="0.0.0.0", port=port)
