# Run the Forest Fire IoT project on Windows

This version has no TFT screen. The remaining ESP32 GPIO pins are unchanged. Node-RED shows live values, InfluxDB stores history, LINE sends state-change alerts, and ThingSpeak provides the public view-only page.

## 1 Prepare local settings

Install and start Docker Desktop, Python 3, and Arduino IDE. Open PowerShell in the folder containing `docker-compose.yml`:

```powershell
Copy-Item .env.example .env
notepad .env
```

Replace `INFLUX_ADMIN_PASSWORD` and `INFLUX_TOKEN` with long unique values. `.env` is ignored by Git. Leave the LINE and ThingSpeak fields blank until those accounts are ready.

## 2 Start the fog stack

```powershell
docker compose up --build -d
docker compose ps
```

`mqtt`, `nodered`, and `influxdb` should be running. Open the private dashboard at `http://localhost:1880/ui`, the Node-RED editor at `http://localhost:1880`, and the InfluxDB UI at `http://localhost:8086`. Sign in to InfluxDB with the admin username and password from `.env`. For startup errors, run `docker compose logs --tail=80 nodered`, replacing `nodered` with another service name as needed.

## 3 Run the digital twin

In another PowerShell window in the same folder:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r twin\requirements.txt
.\.venv\Scripts\python.exe twin\twin.py --scenario demo
```

Select **Use digital twin** on the dashboard. The 90-second demo moves through SAFE, WARNING, ALARM, and recovery to SAFE. The dashboard should show `twin / LIVE` while messages arrive and stale about 15 seconds after they stop. In InfluxDB, inspect the `telemetry` bucket for `forest_fire` records tagged `source=twin`.

## 4 Enable LINE alerts

Create a LINE Official Account and enable its Messaging API. Add the account as a friend on the receiving phone. Copy the channel access token and recipient user ID from the LINE Developers console into the ignored `.env` file as `LINE_CHANNEL_ACCESS_TOKEN` and `LINE_USER_ID`. Restart Node-RED:

```powershell
docker compose up -d --force-recreate nodered
```

Run the twin demo again. The phone should receive one alert on transition to WARNING, another at ALARM, and a recovery message at SAFE. The flow does not send one message per sample. Check the Node-RED warnings if no message arrives. This uses outbound LINE push requests and needs no inbound webhook. Never commit the token or user ID or show them in screenshots.

## 5 Enable the public ThingSpeak channel

Create a public read-only channel with eight fields: temperature C, humidity %, flame flag, motion flag, state code, flame AO, servo degrees, and muted flag. Put its private Write API Key in `THINGSPEAK_WRITE_KEY` in `.env`, then run `docker compose up -d --force-recreate nodered`. Run the twin again and allow at least 20 seconds for a public point. State codes are OFF=0, SAFE=1, WARNING=2, ALARM=3.

## 6 Wire and upload the ESP32

Follow [the current wiring chart](hardware_wiring.svg) and [pin table](HARDWARE_WIRING.md). Do not connect the TFT. Keep DHT11 DATA on GPIO4, flame AO on GPIO34, PIR OUT on GPIO27, button on GPIO13, buzzer on GPIO26, RGB LED on GPIO25/33/32, servo signal on GPIO21, and SD CS on GPIO22. The SD reader uses SCK GPIO18, MOSI GPIO23, and MISO GPIO19. Join all grounds. Power the servo from an external regulated 5 V supply with shared ground.

In Arduino IDE, install ESP32 board support and select the board matching your hardware (the supplied project uses DOIT ESP32 DEVKIT V1). Install DHT sensor library, Adafruit Unified Sensor, ESP32Servo, and PubSubClient. Screen libraries are no longer required. Find the computer's LAN IPv4 address with `ipconfig`. Then:

```powershell
Copy-Item firmware\ForestFireIoT\secrets.example.h firmware\ForestFireIoT\secrets.h
notepad firmware\ForestFireIoT\secrets.h
```

Set the 2.4 GHz Wi-Fi name/password and `MQTT_HOST` to the computer's LAN IPv4 address, not `localhost`. Upload `firmware\ForestFireIoT\ForestFireIoT.ino`. Open Serial Monitor at 115200 baud. Allow Docker MQTT on the trusted private network, TCP port 1883, if Windows Firewall prompts.

## 7 Check hardware and record the demo

Select **Use ESP32** on the dashboard. Compare its values with Serial and InfluxDB; check `esp32 / LIVE`. Insert a FAT32 microSD card and confirm `fire_log.csv` is written. With servo power connected, check the SAFE pointer at 45 degrees. Press **Alarm test ON** and expect ALARM, red LED, buzzer, and servo around 135 degrees. Press **Mute buzzer** to silence only the sound. Press **Alarm test OFF** and allow three seconds to recover. LINE should alert on the state change. A short physical button press mutes, double press toggles battery saver, and long press turns the system on/off.

Record two short videos showing WARNING/recovery and ALARM/control/recovery. Include the device, private dashboard, InfluxDB history, LINE phone alert, and public ThingSpeak page in the evidence. Do not use an uncontrolled flame. Fill the report and slide drafts with genuine results and screenshots.

## Troubleshooting and stopping

| Symptom | Check |
|---|---|
| Dashboard unavailable | `docker compose ps`; `docker compose logs --tail=80 nodered` |
| Twin connection refused | Mosquitto container and TCP port 1883 |
| Dashboard stale | Selected source, twin still running or ESP32 online, LAN firewall |
| No InfluxDB data | Influx token in `.env`, InfluxDB logs, Node-RED warnings |
| No LINE alert | OA friendship, channel token and user ID, Node-RED restart and warnings |
| No ThingSpeak point | Write key, public channel setup, 20-second wait, Node-RED debug |
| Servo resets ESP32 | External regulated 5 V supply and shared ground |

Use `Ctrl+C` to stop the twin and `docker compose down` to stop containers without deleting stored InfluxDB data.
