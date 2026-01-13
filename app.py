from flask import Flask, jsonify, request
import time
import os

app = Flask(__name__)

# --- Robot state (Identical to your original) ---
bot_state = {
    "POS":     {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "time": None},
    "Wall-E": {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "time": None},
    "Dazey":  {"left_wheel": None, "right_wheel": None, "distance": None, "gripper": None, "time": None}
}

@app.route("/")
def index():
    return "Dashboard backend is running"

@app.route("/status")
def status():
    return jsonify({
        "connected": True, 
        "robots": bot_state
    })

# --- This receives the data from your local computer ---
@app.route("/update", methods=["POST"])
def update_data():
    global bot_state
    data = request.json
    bot_name = data.get("bot_name")
    key = data.get("key")
    value = data.get("value")
    
    if bot_name in bot_state:
        if key == "done":
            print(f"{bot_name} is done!")
        else:
            bot_state[bot_name][key] = value
        bot_state[bot_name]["time"] = time.strftime("%H:%M:%S")
        return "OK", 200
    return "Error", 400

if __name__ == "__main__":
    # Render uses the PORT environment variable
    port = int(os.environ.get("PORT", 8000))
    app.run(host="0.0.0.0", port=port)
