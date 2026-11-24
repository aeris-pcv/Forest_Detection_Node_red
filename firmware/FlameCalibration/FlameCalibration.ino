/*
  KY-026 flame sensor calibration for DOIT ESP32 DEVKIT V1.
  No Wi-Fi, MQTT, display, or other sensors required.

  KY-026 AO  -> ESP32 GPIO34 (base board row S)
  KY-026 VCC -> ESP32 3V3 (base board row V ONLY if V is 3.3 V)
  KY-026 GND -> ESP32 GND (base board row G)
  KY-026 DO is not used.

  Serial Monitor: 115200 baud; send 'b' for background, 'f' for a
  supervised test source. Keep the sensor and source at the intended
  operating distance. No open flame is required for wiring checks.
*/

constexpr uint8_t FLAME_AO = 34;
constexpr int SAMPLE_COUNT = 100;
constexpr int SAMPLE_DELAY_MS = 10;

struct SampleRange {
  int low;
  int high;
  int average;
  bool valid;
};

SampleRange background = {0, 0, 0, false};
SampleRange testSource = {0, 0, 0, false};
unsigned long lastLivePrint = 0;

SampleRange capture() {
  int low = 4095;
  int high = 0;
  long total = 0;
  for (int i = 0; i < SAMPLE_COUNT; ++i) {
    const int value = analogRead(FLAME_AO);
    if (value < low) low = value;
    if (value > high) high = value;
    total += value;
    delay(SAMPLE_DELAY_MS);
  }
  return {low, high, static_cast<int>(total / SAMPLE_COUNT), true};
}

void printRange(const char* label, const SampleRange& r) {
  Serial.printf("%s: min=%d max=%d average=%d (0-4095)\n",
                label, r.low, r.high, r.average);
}

void suggestThreshold() {
  if (!background.valid || !testSource.valid) return;

  if (background.high == 0 || testSource.high == 0) {
    Serial.println("AO stays at 0: check 3V3, GND, AO->GPIO34, and base-board row alignment.");
    return;
  }
  if (testSource.high < background.low) {
    const int threshold = (testSource.high + background.low) / 2;
    Serial.printf("Suggested FLAME_ANALOG_MAX = %d\n", threshold);
    Serial.println("Confirm repeated samples still separate before changing the main sketch.");
  } else if (background.high < testSource.low) {
    Serial.println("Test source makes AO HIGHER, but main firmware assumes LOWER means flame.");
    Serial.println("Check the module and its labels before changing the detection rule.");
  } else {
    Serial.println("Ranges overlap. Change distance/lighting or adjust the module; no reliable threshold yet.");
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Serial.println("KY-026 AO calibration on GPIO34");
  Serial.println("Type b + Send in normal lighting; then f + Send at the safe test source.");
}

void loop() {
  if (Serial.available()) {
    const char command = static_cast<char>(Serial.read());
    if (command == 'b' || command == 'B') {
      background = capture();
      printRange("Background", background);
      suggestThreshold();
    } else if (command == 'f' || command == 'F') {
      testSource = capture();
      printRange("Test source", testSource);
      suggestThreshold();
    }
  }

  if (millis() - lastLivePrint >= 2000) {
    lastLivePrint = millis();
    const int ao = analogRead(FLAME_AO);
    Serial.printf("Live AO=%d\n", ao);
    if (ao == 0) Serial.println("AO=0: verify sensor power, ground, and AO signal wire.");
  }
}
