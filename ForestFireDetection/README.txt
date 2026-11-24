INSTRUCTIONS TO INSTALL

1. ESP32 board support

   a. Go to File > Preferences. Under "Additional boards manager URLs", add:
      https://espressif.github.io/arduino-esp32/package_esp32_index.json
   b. Go to Tools > Board > Boards Manager, search for "esp32" (by Espressif Systems), and click Install.
   c. Select Tools > Board > DOIT ESP32 DEVKIT V1.

2. Libraries (Sketch > Include Library > Manage Libraries, or Ctrl+Shift+I)

   Install each of these:
   - Adafruit GFX Library (Adafruit)
   - Adafruit ILI9341 (Adafruit)
   - Adafruit BusIO (Adafruit)
   - DHT sensor library (Adafruit)
   - Adafruit Unified Sensor (Adafruit)
   - ESP32Servo (Kevin Harrington, John K. Bennett)

   If the IDE asks to install dependencies for a library, click "Install all".

3. Built-in libraries (no installation needed)

   WiFi, time, SPI and SD come with the ESP32 board package.

4. Before uploading

   - Set your WiFi name and password in WIFI_SSID and WIFI_PASS. Use a 2.4 GHz network.
   - Choose the correct port under Tools > Port.
   - Open the Serial Monitor at a baud rate of 115200.
   - Format the microSD card as FAT32 (32 GB or smaller).

5. Before sharing the code

   Replace the WiFi name and password in the code with placeholders:
   WIFI_SSID = "YOUR_WIFI_NAME"
   WIFI_PASS = "YOUR_WIFI_PASSWORD"
