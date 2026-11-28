# Forest Fire Detection IoT System

Project 2 extension of the supplied ESP32 forest fire detector. The same telemetry contract runs on the physical ESP32 and an ESP32-free digital twin. Mosquitto and Node-RED form the local fog layer. InfluxDB stores telemetry, LINE Messaging API sends state-change alerts, and ThingSpeak remains the public view-only dashboard. The physical TFT has been removed; all other GPIO assignments are unchanged.

For complete Windows instructions, follow [`docs/RUN_STEP_BY_STEP.md`](docs/RUN_STEP_BY_STEP.md).

See the [conventional flowchart diagram](docs/system_flowchart_diagram.svg) for the device decisions and data path through MQTT, Node-RED, InfluxDB, LINE and ThingSpeak.

If the board logs `ALARM` with `flame_ao=0` or the DHT readings are unavailable, upload [`firmware/SensorWiringTest/SensorWiringTest.ino`](firmware/SensorWiringTest/SensorWiringTest.ino). It prints raw sensor readings without starting Wi-Fi or actuators.

To choose a flame threshold for your actual module and lighting, upload [`firmware/FlameCalibration/FlameCalibration.ino`](firmware/FlameCalibration/FlameCalibration.ino). Use Serial Monitor at 115200 baud, send `b` in normal lighting, then `f` at a supervised test source. Copy the suggested value into `FLAME_ANALOG_MAX` in the main firmware only after both ranges are clearly separated.

To check the microSD reader alone, upload [`firmware/SDCardTest/SDCardTest.ino`](firmware/SDCardTest/SDCardTest.ino). It uses the same GPIO18/19/23/22 wiring as the main sketch, appends one line to `/sd_test.txt`, reads it back, and prints PASS or FAIL at 115200 baud.

## Quick start

1. Install Docker Desktop. Copy `.env.example` to `.env` and set a strong InfluxDB password and token. Run `docker compose up --build -d` from this directory.
2. Open `http://localhost:1880/ui` for the private dashboard. Select **Use digital twin**.
3. Install the twin dependency with `python -m pip install -r twin/requirements.txt`, then run `python twin/twin.py --scenario demo`.
4. Inspect SAFE, WARNING, ALARM, recovery, recommendation, freshness, and control buttons. The Node-RED editor is at `http://localhost:1880`.
5. Open `http://localhost:8086` to view stored telemetry. Add a LINE Official Account channel access token and recipient user ID to `.env` to enable state-change alerts. Create a ThingSpeak channel with eight fields, set it to public read access, add the private write key to `.env`, and restart Node-RED with `docker compose up -d --force-recreate nodered`.
6. Copy `firmware/ForestFireIoT/secrets.example.h` to `firmware/ForestFireIoT/secrets.h`, then set `WIFI_SSID`, `WIFI_PASS`, and `MQTT_HOST` in the new file. The MQTT host is the laptop's LAN IPv4 address, not `localhost`. Upload the sketch to a DOIT ESP32 DevKit V1. Install the original Project 1 libraries plus `PubSubClient`.
7. Select **Use ESP32** on the dashboard. Confirm live samples and commands. The physical button retains the Project 1 behavior: short press mute, double press battery saver, long press on/off.

## Topics and contract

| Topic | Direction | Payload |
|---|---|---|
| `forestfire/esp32/telemetry` | ESP32 to fog | JSON sample, retained |
| `forestfire/twin/telemetry` | twin to fog | Same JSON sample, retained |
| `forestfire/{source}/command` | fog to device/twin | `MUTE`, `UNMUTE`, `TEST_ON`, `TEST_OFF` |
| `forestfire/esp32/status` | ESP32 to fog | retained `online` / MQTT last-will `offline` |

The JSON fields are `source`, `timestamp`, `temperature_c`, `humidity`, `flame_ao`, `flame`, `motion`, `state`, `muted`, `servo_deg`, and `battery_saver`. DHT errors transmit `null` readings. The device samples flame/PIR at 10 Hz, DHT every 2 seconds, and publishes telemetry every 5 seconds. In battery saver, local sampling slows, but alarm response still runs continuously. ThingSpeak accepts updates at most once every 16 seconds.

## Safety and deployment limits

This is an educational prototype, not a certified fire alarm. Keep the broker on a trusted classroom LAN; its demo configuration allows anonymous MQTT and does not expose Node-RED or InfluxDB beyond localhost. Do not expose port 1883 to the public internet. ThingSpeak is public read-only; the ThingSpeak write key, LINE token and user ID, and InfluxDB credentials stay in ignored local `.env` and are never sent to the browser. Local flame/heat sensing controls the alarm even if Wi-Fi or Node-RED is down. The remote test can raise an alarm but cannot override a real danger reading.

The dashboard and cloud publishing need a running Docker host and a network connection. Check the current wiring in [`docs/HARDWARE_WIRING.md`](docs/HARDWARE_WIRING.md) and follow [`docs/DEMO.md`](docs/DEMO.md) to record the results for submission.
