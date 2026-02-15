import serial
import requests
import time

# --- CONFIG ---
ARDUINO_PORT = "/dev/tty.usbmodem48CA435A9B082"
RENDER_URL = "https://whiskas-website.onrender.com/"
BAUD_RATE = 9600

# Your hard-earned mapping logic
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

try:
    ser = serial.Serial(ARDUINO_PORT, BAUD_RATE, timeout=0.05)
    print("Connected to Arduino. Forwarding to Render...")
except:
    print("Could not find Arduino locally.")
    exit()

while True:
    if ser.in_waiting >= 2:
        packet = ser.read(2)
        val, head = packet[0], packet[1]
        
        if head in HEADER_MAP:
            bot, key = HEADER_MAP[head]
            try:
                requests.post(RENDER_URL, json={"bot": bot, "key": key, "val": val})
                print(f"Sent: {bot} {key}={val}")
            except:
                print("Cloud offline")
    time.sleep(0.05)
