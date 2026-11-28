# Live validation and presentation checklist

## Preflight

1. Photograph the assembled hardware and confirm pin connections against `docs/HARDWARE_WIRING.md`. Calibrate KY-026 AO in normal lighting and under a safe simulated source; adjust `FLAME_ANALOG_MAX` if necessary.
2. Run Docker Compose. In the Node-RED editor, check for connected MQTT nodes and no flow errors. Open the private dashboard.
3. Run the twin with `--scenario demo`. Capture its dashboard in SAFE, WARNING, ALARM, and recovery. Confirm InfluxDB history and LINE state-change alerts. Press mute and alarm test buttons; confirm twin messages change.
4. Create a public ThingSpeak channel, set field names, enable public read access, configure `.env`, and verify new points appear. Record the channel URL.
5. Upload ESP32 firmware, select the physical source, and compare dashboard values with Serial, InfluxDB, and SD CSV. Confirm the freshness indicator changes to stale after disconnecting the device.

## Two required short videos

**Video 1: warning and recovery, 20-30 seconds.** Show the dashboard and physical LED/servo. Safely warm the DHT11 or use the twin with clear on-screen labeling. Show WARNING recommendation and subsequent SAFE state. Record actual measured values and timestamps.

**Video 2: alarm, control, and cloud, 20-30 seconds.** Use the admin alarm-test button or a safe flame-sensor test. Show ALARM indication, buzzer and servo, mute control, LINE alert, InfluxDB history, public ThingSpeak read-only view, and TEST_OFF/recovery. Never use an uncontrolled flame indoors.

## Report and slides

Complete the project documentation DOCX locally with the current circuit and FSM diagrams, a Node-RED flow screenshot, private dashboard screenshots, public channel screenshot, measured scenario results, group names/photos, and version history. Export the final report to PDF.

Prepare about eight slides: title/team; problem and objectives; physical circuit; architecture/protocol; FSM and analytics; private and public dashboards; two scenario results with video links; conclusions and limitations. Export to PDF. Submit that PDF, the report PDF, and a ZIP of `firmware`, `fog`, `twin`, `docker-compose.yml`, `.env.example`, and documentation. Exclude real credentials and `.env`.
