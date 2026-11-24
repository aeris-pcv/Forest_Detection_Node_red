# ESP32 hardware wiring

Use the [clean schematic](hardware_wiring.svg) with the Project 2 sketch. The supplied Project 1 Fritzing picture does not show the PIR, so GPIO27 has been added here. No network wire is required: the ESP32 connects to the Docker MQTT broker over 2.4 GHz Wi-Fi.

| Module | Module pin | ESP32 / supply connection |
|---|---|---|
| DHT11 module | VCC, DATA, GND | 3V3, GPIO4, GND |
| KY-026 flame | VCC, AO, DO, GND | 3V3, GPIO34, GPIO35, GND. DO is diagnostic; current code uses AO only. |
| HC-SR501 PIR | VCC, OUT, GND | 5V/VIN, GPIO27, GND. Confirm OUT is 3.3 V compatible on your module. |
| Separate microSD module | VCC, GND, SCK, MOSI, MISO, CS | 3V3, GND, GPIO18, GPIO23, GPIO19, GPIO22 |
| Active buzzer | +, - | GPIO26, GND, if the buzzer current is within the GPIO rating; otherwise use a transistor driver |
| RGB LED, common cathode | R, G, B, common | GPIO25, GPIO33, GPIO32 through one 220 ohm resistor per color; common to GND |
| Push button | two opposite contacts | GPIO13 and GND; firmware enables the internal pull-up |
| Servo | signal, +, - | GPIO21, external regulated 5 V, common GND |

The screen is removed. The separate microSD reader keeps its original SPI pins: SCK GPIO18, MOSI GPIO23, MISO GPIO19, CS GPIO22. GPIO5, GPIO16, and GPIO17 are now unused.

**Power:** Connect every module ground and the external servo supply ground to ESP32 GND. Do not route servo power through the ESP32 3V3 pin. Use a regulated 5 V source sized for the servo's stall current. Check the actual SD, PIR, and buzzer board labels and voltage ratings before applying power. A bare DHT11 sensor needs a DATA pull-up (typically 10 kohm to 3V3); many three-pin modules include it.

**Assembly order:** Build the 3V3/GND rails first, then DHT11, flame, PIR, button, indicators, microSD, and servo last. With servo power disconnected, upload the sketch and verify Serial readings. Then power the servo and check SAFE/WARNING/ALARM positions. Set `MQTT_HOST` in `firmware/ForestFireIoT/secrets.h` to the Docker laptop's LAN address.
