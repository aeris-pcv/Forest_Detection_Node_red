# Run the Forest Fire IoT project on Windows

Follow these steps from the project root, the folder containing `docker-compose.yml`. Start with the digital twin, which needs no ESP32. Then connect and upload the hardware. Power the servo only after checking the wiring.

## 1. Prepare the computer

Install and start Docker Desktop, Python 3 with `pip`, and Arduino IDE. The computer must have internet access for the first Docker image build and for ThingSpeak. The ESP32 and computer must be on the same local network; use a 2.4 GHz Wi-Fi network supported by the ESP32. Keep the computer awake while demonstrating the system.

Open **PowerShell** in the project root. Check the tools:

```powershell
docker --version
docker compose version
python --version
```

If `docker` is unavailable, start Docker Desktop and reopen PowerShell. If `python` is unavailable, install Python and reopen PowerShell. If using the submitted ZIP, extract it first and open PowerShell in the extracted folder.

## 2. Start the local MQTT and Node-RED services

```powershell
docker compose up --build -d
docker compose ps
```

Both `mqtt` and `nodered` should show **running**. The first build downloads images and installs the Node-RED dashboard package, so it can take several minutes. Check startup messages if one container stops:

```powershell
docker compose logs --tail=80 mqtt
docker compose logs --tail=80 nodered
```

Open these pages on the **same computer**:

- Admin dashboard: `http://localhost:1880/ui`
- Node-RED flow editor: `http://localhost:1880`

The dashboard is local to this computer. It initially has no live measurements. Node-RED should show the **Forest fire fog** flow. Select **Use digital twin** on the dashboard.

## 3. Run the ESP32-free digital twin

In a second PowerShell window, opened in the project root, create a Python virtual environment and install its one dependency:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r twin\requirements.txt
```

Run the complete scenario sequence:

```powershell
.\.venv\Scripts\python.exe twin\twin.py --scenario demo
```

The terminal prints JSON samples. On `http://localhost:1880/ui`, expect the following sequence over roughly 90 seconds:

| Phase | Twin input | Expected dashboard state |
|---|---|---|
| Normal | 27.5 C, 65% humidity, no flame | SAFE |
| Hot and dry | 39 C, 25% humidity, no flame | WARNING |
| Flame / heat | 47 C, flame AO 1100, movement | ALARM, PERSON NEAR |
| Clear | 28 C, 60% humidity, no flame | SAFE after about 3 seconds |

The source/freshness text should show `twin / LIVE` while samples arrive. After the twin exits, it changes to `STALE OR OFFLINE` after about 15 seconds. The last reading may remain visible; freshness tells you whether it is current.

For a longer single scene, run `--scenario safe`, `--scenario warning`, `--scenario alarm`, or `--scenario recovery` instead of `demo`. To test admin controls, run a scenario and press **Mute buzzer**, **Unmute buzzer**, **Alarm test ON**, then **Alarm test OFF**. The twin has no physical buzzer, but its `muted` and `state` fields change. The physical ESP32 responds with real actuators later.

## 4. Configure the optional public ThingSpeak view

Create a ThingSpeak channel with public read access and these eight fields: `Temperature C`, `Humidity %`, `Flame flag`, `Motion flag`, `State code`, `Flame AO`, `Servo degrees`, and `Muted flag`. Keep its **Write API Key** private. Copy the sample environment file and edit the new file:

```powershell
Copy-Item .env.example .env
notepad .env
```

Set `THINGSPEAK_WRITE_KEY` to your channel's Write API Key, with no quotes or spaces around `=`. Restart Node-RED so it receives the new environment value:

```powershell
docker compose up -d --force-recreate nodered
```

Run the twin again. Open your channel's public view and allow at least 20 seconds for a point. The flow uploads no more often than every 16 seconds. State codes are `0=OFF`, `1=SAFE`, `2=WARNING`, `3=ALARM`. If no point appears, check that `.env` contains a real key and inspect Node-RED's **Cloud response/errors** debug output. Do not include `.env` in the submission ZIP.

## 5. Connect the ESP32 hardware

Use [the wiring chart](hardware_wiring.svg) and [pin table](HARDWARE_WIRING.md). The original Fritzing drawing omitted the PIR; connect its OUT pin to **GPIO27**. Connect all grounds together. The servo signal goes to GPIO21, but its positive supply should come from an external regulated 5 V source with common ground. Connect the TFT and separate SD reader to the shared SPI pins GPIO18 and GPIO23, with separate CS pins GPIO5 and GPIO22.

Before applying power, compare every module's actual printed pin labels with the chart. Check that the RGB LED is **common cathode** and each color has its own 220 ohm resistor. Keep servo power disconnected for the first upload.

## 6. Set up Arduino IDE and upload

Install the ESP32 board package in Arduino IDE using the board manager URL from the supplied Project 1 README:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Select the board matching the physical module (the supplied project targets **DOIT ESP32 DEVKIT V1**) and its serial port. Install these libraries in Library Manager: **Adafruit GFX Library**, **Adafruit ILI9341**, **Adafruit BusIO**, **DHT sensor library**, **Adafruit Unified Sensor**, **ESP32Servo**, and **PubSubClient**. WiFi, SPI, and SD come with the ESP32 board package.

Find the computer's local IPv4 address:

```powershell
ipconfig
```

Create your local firmware settings from the example, then edit them:

```powershell
Copy-Item firmware\ForestFireIoT\secrets.example.h firmware\ForestFireIoT\secrets.h
notepad firmware\ForestFireIoT\secrets.h
```

Replace the placeholder Wi-Fi name, password, and `MQTT_HOST` with your computer's local IPv4 address, for example `192.168.1.10`. **Do not use `localhost`** as the MQTT host on the ESP32. Open `firmware\ForestFireIoT\ForestFireIoT.ino` in Arduino IDE, compile, and upload it. Open Serial Monitor at **115200 baud**. Git ignores `secrets.h`; commit only `secrets.example.h`.

Expected signs of life: the TFT displays the current state and measurements; Serial prints Wi-Fi/NTP status and CSV rows; after a few seconds Node-RED receives MQTT telemetry. If Windows Firewall prompts for Docker's network access, allow it on the trusted private network so the ESP32 can reach TCP port 1883.

## 7. Verify physical dashboard and controls

On the dashboard, press **Use ESP32**. Within about five seconds it should show measured temperature, humidity, flame and state; freshness should show `esp32 / LIVE`. Compare readings with the TFT. With servo power connected, its pointer should move to SAFE at 45 degrees. Insert a FAT32 microSD card and check that `fire_log.csv` is created.

Test the controls in a safe setting:

1. Press **Alarm test ON**. The ESP32 should enter ALARM; LED red, buzzer sounding, servo toward 135 degrees.
2. Press **Mute buzzer**. Sound stops, but the ALARM state remains.
3. Press **Unmute buzzer**, then **Alarm test OFF**. After three seconds of no real danger, it should return to SAFE or WARNING.
4. Short press the physical button to mute/unmute; double press for battery saver; long press for system on/off.
5. Unplug the ESP32 or disable its Wi-Fi briefly. The dashboard should become stale, while the local detector continues sensing and actuating. Reconnect and check it becomes live again.

Do not use an uncontrolled flame to test the sensor. Calibrate the KY-026 analog threshold for the actual room lighting before claiming reliable flame detection.

## 8. Capture the graded evidence

Record two short videos: one showing WARNING then recovery, and one showing ALARM, admin mute, public view, and recovery. Capture the physical device and dashboard together where possible. Take screenshots of the Node-RED flow, private dashboard, public ThingSpeak channel, and the two results. Record the actual observed values and timestamps. Fill the supplied DOCX template with the diagrams, screenshots, group details, and results; export it and the approximately eight presentation slides to PDF. The [demo checklist](DEMO.md) lists the required evidence.

## 9. Stop or restart

To stop the twin, use `Ctrl+C` in its PowerShell window. To stop the containers without deleting their data:

```powershell
docker compose down
```

To run again later, open PowerShell in the project root and use `docker compose up -d`, then run the twin or turn on the ESP32. Keep `.env` and real Wi-Fi credentials out of the submitted ZIP.

## Quick troubleshooting

| Symptom | Check |
|---|---|
| Dashboard page does not open | `docker compose ps`; inspect `docker compose logs --tail=80 nodered` |
| Twin prints connection refused | Mosquitto container and port 1883; `docker compose logs --tail=80 mqtt` |
| Dashboard says stale | Correct source selected; twin still running or ESP32 powered and connected; wait one telemetry interval |
| ESP32 has no cloud data | Computer LAN IPv4 in `secrets.h`, same network, firewall TCP 1883, Node-RED MQTT connected |
| No ThingSpeak points | Correct Write API Key in `.env`; restarted Node-RED; allow 20 seconds; inspect debug panel |
| Servo resets ESP32 | Use a suitable external 5 V supply and shared ground; check wiring |
| SD does not log | FAT32 card, SD CS GPIO22, SPI MISO GPIO19, common ground |
| Flame always active | Read AO in Serial diagnostics and recalibrate `FLAME_ANALOG_MAX`; leave DO disabled unless tested |
