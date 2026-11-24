# Eight slide presentation draft

## 1 Forest Fire Detection and Alert IoT System

ITCS447 Project 2. Add group number, members, photos, and date. Opening sentence: Our ESP32 senses local fire risk, acts immediately, and shares the result through a local fog dashboard and a public cloud channel.

## 2 Problem and objective

Nearby flame and heat need site-specific sensing. Goals: detect SAFE/WARNING/ALARM, alert locally, give an admin remote control, and publish public view-only data. Show two scenarios: hot/dry warning and flame/severe heat alarm.

## 3 Hardware and circuit

Insert the supplied Fritzing schematic and a real hardware photo. Label DHT11, KY-026, PIR, TFT, RGB LED, buzzer, servo, SD, and interrupt button. Explain that the ESP32 continues sensing and actuating if the network fails.

## 4 Architecture and protocol

Insert the Mermaid architecture from `REPORT.md` as an image. ESP32 and twin send JSON over local Wi-Fi/MQTT to Mosquitto; Node-RED provides fog analytics and admin control; HTTPS publishes numeric fields to the public ThingSpeak channel. Explain five-second telemetry and 16-second cloud rate limit.

## 5 Decision logic

Insert the FSM diagram. WARNING: temperature at least 38 C, or at least 33 C with humidity at most 30%. ALARM: flame AO below calibrated 2000, or temperature at least 45 C. Danger must clear for three seconds before leaving ALARM. Muting does not change the state.

## 6 Dashboards and digital twin

Insert actual screenshots of Node-RED's private dashboard and ThingSpeak's public channel. Show source selection, recommendation, freshness, and control buttons. Explain that the twin uses the same schema and thresholds for ESP32-free evaluation.

## 7 Scenario results and demo

Show a small table with observed temperature, humidity, flame AO, state, LED/servo, and cloud update for each of the two recorded scenarios. Embed public links to the two 20-30-second videos. Clearly label which evidence comes from the twin and which comes from hardware.

## 8 Conclusion and limits

Summarize achieved outcomes after live verification. Note calibration and Wi-Fi dependence for remote visibility, while local detection remains independent. State what the group learned and the next improvement: stronger broker authentication and an outdoor-ready long-range backhaul.
