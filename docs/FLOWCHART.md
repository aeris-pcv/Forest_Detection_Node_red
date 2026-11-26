# Forest Fire IoT flowchart

![Forest Fire IoT infographic flowchart](flowchart_infographic.svg)

[Download the PNG version](flowchart_infographic.png) for slides or the report.

This flowchart follows the firmware and Node-RED flow. GitHub renders the Mermaid diagram below directly in this page.

```mermaid
flowchart TD
    Start([Power on ESP32]) --> Init[Initialize sensors, outputs, SD card, Wi-Fi and time]
    Init --> Read[Read flame, PIR, temperature, humidity and button]
    Read --> Long{Long button press?}
    Long -- Yes --> Power{System currently OFF?}
    Power -- Yes --> On[Turn system ON]
    Power -- No --> Off[Log SYSTEM_OFF, stop outputs, park servo]
    Off --> Wait[Wait for button while OFF]
    Wait --> Read
    On --> Read
    Long -- No --> Short{Short or double press?}
    Short -- Short --> Mute[Toggle buzzer mute]
    Short -- Double --> Saver[Toggle battery saver]
    Short -- Neither --> Danger
    Mute --> Danger
    Saver --> Danger
    Danger{Flame, temperature at least 45 C, or remote alarm test?}
    Danger -- Yes --> Alarm[ALARM: red LED, buzzer unless muted, servo 135 degrees]
    Danger -- No --> Hold{Was ALARM active in last 3 seconds?}
    Hold -- Yes --> Alarm
    Hold -- No --> Warning{Temperature at least 38 C, or temperature at least 33 C and humidity at most 30 percent?}
    Warning -- Yes --> Warn[WARNING: warning outputs, servo 90 degrees]
    Warning -- No --> Safe[SAFE: normal outputs, servo 45 degrees]
    Alarm --> Log[Print Serial event and save CSV to SD if available]
    Warn --> Log
    Safe --> Log
    Log --> Network{Wi-Fi and MQTT connected?}
    Network -- No --> Read
    Network -- Yes --> Publish[Publish JSON every 5 seconds to forestfire/esp32/telemetry]
    Publish --> Broker[Mosquitto MQTT broker]
    Broker --> Validate[Node-RED validates data and selects ESP32 or digital twin]
    Validate --> Dashboard[Private dashboard and recommendations]
    Validate --> Influx[Store selected telemetry in InfluxDB]
    Validate --> Change{State changed?}
    Change -- Yes --> Line[Push LINE alert for WARNING, ALARM or recovery]
    Change -- No --> Cloud
    Line --> Cloud{At least 16 seconds since cloud update?}
    Cloud -- Yes --> ThingSpeak[Update public ThingSpeak channel]
    Cloud -- No --> Read
    ThingSpeak --> Read
    Dashboard --> Commands[Admin mute, unmute and alarm test commands]
    Commands --> Broker
    Broker --> Read
```

The physical alarm and SD logging continue if Wi-Fi is unavailable. The digital twin publishes the same JSON format on `forestfire/twin/telemetry`. Node-RED processes whichever source is selected on the private dashboard; only selected-source records go to InfluxDB, LINE and ThingSpeak.

Flame detection uses the board's calibrated `flame` result, so this diagram remains valid if the sensor's analog threshold or polarity is adjusted for the actual module. A failed DHT read does not create a heat alarm; flame detection still runs.
