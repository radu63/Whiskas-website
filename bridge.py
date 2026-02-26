import serial
import requests
import time

PORT = "COM6"
BAUD = 9600
SERVER = "https://whiskas-website-yhh7.onrender.com/update"

HEADER_MAP = {
    129: ("POS", "left_wheel"),
    130: ("POS", "right_wheel"),
    131: ("POS", "gripper"),
    133: ("POS", "distance"),

    65:  ("Wall-E", "left_wheel"),
    66:  ("Wall-E", "right_wheel"),
    67:  ("Wall-E", "gripper"),
    69:  ("Wall-E", "distance"),

    33:  ("Dazey", "left_wheel"),
    34:  ("Dazey", "right_wheel"),
    35:  ("Dazey", "gripper"),
    37:  ("Dazey", "distance"),
}

ser = serial.Serial(PORT, BAUD, timeout=0.1)

print("Bridge running...")

while True:

    if ser.in_waiting >= 2:

        header = ser.read(1)[0]
        value = ser.read(1)[0]

        if header in HEADER_MAP:

            bot, key = HEADER_MAP[header]

            try:
                response = requests.post(
                SERVER,
                json={
                "bot": bot,
                "key": key,
                "val": value,
                "api_key": "NHL.Bot123"
            }
        )

                print(bot, key, value)
                print("HTTP:", response.status_code)

            except Exception as e:
                print("POST ERROR:", e)

    time.sleep(0.05)