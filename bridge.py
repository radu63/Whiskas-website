import serial
import requests
import time

PORT = "COM6"   # anpassen
BAUD = 9600
SERVER = "https://whiskas-website-yhh7.onrender.com/update"

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

ser = serial.Serial(PORT, BAUD, timeout=0.1)

print("Connected to", PORT)
print("Bridge running...")

while True:
    print("in_waiting:", ser.in_waiting)
    
    if ser.in_waiting >= 2:
        header = ser.read(1)[0]
        value = ser.read(1)[0]

        if header in HEADER_MAP:
            bot, key = HEADER_MAP[header]

            try:
                requests.post(SERVER, json={
                    "bot": bot,
                    "key": key,
                    "val": value
                })
                print(bot, key, value)
            except:
                print("Server not reachable")

    time.sleep(0.05)