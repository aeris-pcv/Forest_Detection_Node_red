"""ESP32-free digital twin for the four operational scenarios.

Run with `python twin/twin.py --scenario demo` after Docker Compose starts.
"""
import argparse
import json
import time
from datetime import datetime, timezone


TOPIC = "forestfire/twin/telemetry"
COMMAND = "forestfire/twin/command"
SCENARIOS = {
    "safe": [(27.5, 65, 4095, False, 20)],
    "warning": [(27.5, 65, 4095, False, 10), (39, 25, 4095, False, 25)],
    "alarm": [(27.5, 65, 4095, False, 10), (47, 22, 1100, True, 30)],
    "recovery": [(47, 22, 1100, True, 15), (28, 60, 4095, False, 25)],
}
SCENARIOS["demo"] = SCENARIOS["warning"] + SCENARIOS["alarm"][1:] + SCENARIOS["recovery"][1:]

class Twin:
    def __init__(self):
        self.muted = False
        self.test = False
        self.state = "SAFE"
        self.last_danger = 0.0

    def command(self, _client, _userdata, message):
        command = message.payload.decode("ascii", errors="ignore")
        if command == "MUTE": self.muted = True
        elif command == "UNMUTE": self.muted = False
        elif command == "TEST_ON": self.test = True
        elif command == "TEST_OFF": self.test = False

    def sample(self, temperature, humidity, flame_ao, motion, now=None):
        now = time.monotonic() if now is None else now
        flame = flame_ao < 2000
        danger = flame or temperature >= 45 or self.test
        warning = temperature >= 38 or (temperature >= 33 and humidity <= 30)
        if danger: self.last_danger = now
        if danger: self.state = "ALARM"
        elif self.state == "ALARM" and now - self.last_danger < 3: pass
        else: self.state = "WARNING" if warning else "SAFE"
        return {
            "source": "twin", "timestamp": datetime.now(timezone.utc).isoformat(),
            "temperature_c": temperature, "humidity": humidity,
            "flame_ao": flame_ao, "flame": flame, "motion": motion,
            "state": self.state, "muted": self.muted,
            "servo_deg": {"SAFE": 45, "WARNING": 90, "ALARM": 135}[self.state],
            "battery_saver": False,
        }

def main():
    import paho.mqtt.client as mqtt
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--scenario", choices=SCENARIOS, default="demo")
    parser.add_argument("--interval", type=float, default=2.0)
    args = parser.parse_args()
    twin = Twin()
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="forestfire-twin")
    client.on_message = twin.command
    client.connect(args.host, 1883, 60)
    client.subscribe(COMMAND)
    client.loop_start()
    try:
        for temperature, humidity, flame_ao, motion, duration in SCENARIOS[args.scenario]:
            until = time.monotonic() + duration
            while time.monotonic() < until:
                payload = twin.sample(temperature, humidity, flame_ao, motion)
                client.publish(TOPIC, json.dumps(payload), qos=0, retain=True)
                print(json.dumps(payload), flush=True)
                time.sleep(args.interval)
    finally:
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
