# Forest Fire IoT Project 2 plan

## Scoring map

| Rubric | Deliverable | Verification |
|---|---|---|
| Objective, idea, design (30) | Two operational scenarios, architecture, MQTT rationale, circuit and FSM diagrams, digital twin comparison | Review `docs/REPORT.md` and diagrams against the supplied template |
| Implementation and integration (30) | ESP32 sensors and actuators, interrupt button, SD storage, MQTT, Node-RED analytics and private control, public ThingSpeak channel | Replay simulator scenarios, inspect MQTT messages and dashboard, hardware demo |
| Results and presentation (30) | Scenario evidence, screenshots, two short videos, 8-slide PDF, final PDF report | Capture from the real running system and check links |
| Polish and creativity (10) | Clear operator modes, stale-data indication, alarm priority, recovery logic, reproducible setup | End-to-end test and presentation rehearsal |

## Implementation sequence

1. Preserve the Project 1 hardware pin map and safety behavior.
2. Add nonblocking MQTT telemetry and command handling to the ESP32. Local flame/heat detection always has priority; remote control may mute or request an alarm test, never suppress a real alarm.
3. Build a Docker fog stack: Mosquitto and Node-RED. Use Node-RED for MQTT ingestion, dashboard, history, threshold recommendations, commands, and ThingSpeak upload.
4. Build a digital twin that publishes the same telemetry schema without the ESP32 and exercises safe, warning, alarm, and recovery scenarios.
5. Verify message contract and simulator behavior locally. Then validate physical sensors, actuator, LAN MQTT, SD logging, private dashboard, and public channel on the actual equipment.
6. Capture real screenshots and two short scenario videos. Fill the group report template, create approximately eight slides, export both to PDF, and submit the code ZIP, slide PDF, and report PDF.

## Evidence still requiring the group

Physical ESP32 hardware, Wi-Fi credentials, a ThingSpeak channel and write key, member names/photos, real screenshots/video, and a live presentation cannot be fabricated. The project includes exact setup and capture instructions so these can be completed efficiently.
