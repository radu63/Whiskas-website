import requests
import time

# CONFIG
RENDER_URL = "https://whiskas-website.onrender.com/update"

# Mapping stays untouched
HEADER_MAP = {
    0b00100001: ("Dazey", "left_wheel"), 0b00100010: ("Dazey", "right_wheel"),
    0b00100011: ("Dazey", "gripper"), 0b00100100: ("Dazey", "linestate"),
    0b00100101: ("Dazey", "distance"), 0b00101111: ("Dazey", "done"),
    0b10000001: ("POS", "left_wheel"), 0b10000010: ("POS", "right_wheel"),
    0b10000011: ("POS", "gripper"), 0b10000100: ("POS", "linestate"),
    0b10000101: ("POS", "distance"), 0b10001111: ("POS", "done"),
    0b01000001: ("Wall-E", "left_wheel"), 0b01000010: ("Wall-E", "right_wheel"),
    0b01000011: ("Wall-E", "gripper"), 0b01000100: ("Wall-E", "linestate"),
    0b01000101: ("Wall-E", "distance"), 0b01001111: ("Wall-E", "done"),
}

# Simple send helper
def send(bot, key, val):
    try:
        requests.post(RENDER_URL, json={
            "bot": bot,
            "key": key,
            "val": val
        })
        print(f"Sent: {bot} {key}={val}")
    except:
        print("Cloud offline")


# SIMULATION

def simulate_pos():
    send("POS", "linestate", 0)  # flag blocking
    time.sleep(2)

    send("POS", "linestate", 1)  # blind drive
    time.sleep(2)

    send("POS", "linestate", 2)  # line detected
    time.sleep(1)

    start = time.time()
    toggle = True

    while time.time() - start < 4:
        if toggle:
            send("POS", "left_wheel", 1)
            send("POS", "right_wheel", 0)
        else:
            send("POS", "left_wheel", 1)
            send("POS", "right_wheel", 1)

        toggle = not toggle
        time.sleep(0.2)

    send("POS", "distance", 1)  # obstacle
    time.sleep(2)

    send("POS", "gripper", 1)  # square placed
    time.sleep(1)

    send("POS", "done", 1)
    time.sleep(2)


def simulate_walle():
    send("Wall-E", "linestate", 1)
    time.sleep(2)

    send("Wall-E", "gripper", 1)
    time.sleep(1)

    send("Wall-E", "done", 1)
    time.sleep(2)


def simulate_dazey():
    send("Dazey", "linestate", 1)
    time.sleep(2)

    send("Dazey", "gripper", 1)
    time.sleep(1)

    send("Dazey", "done", 1)
    time.sleep(2)


# --- MAIN ---
while True:
    simulate_pos()
    simulate_walle()
    simulate_dazey()
    break
