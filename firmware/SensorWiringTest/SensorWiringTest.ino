/*
  Wiring-only diagnostic: no Wi-Fi, MQTT, servo, buzzer, or TFT.
  Board: DOIT ESP32 DEVKIT V1
  Install: DHT sensor library and Adafruit Unified Sensor.

  KY-026: VCC -> 3V3, GND -> GND, AO -> GPIO34, DO -> GPIO35 (optional)
  DHT11:  VCC -> 3V3, GND -> GND, DATA -> GPIO4
  PIR:    OUT -> GPIO27, plus VCC and GND per your module's rating
*/
#include <DHT.h>

constexpr int DHT_PIN = 4;
constexpr int FLAME_AO = 34;
constexpr int FLAME_DO = 35;
constexpr int PIR_PIN = 27;
DHT dht(DHT_PIN, DHT11);

void setup() {
  Serial.begin(115200);
  pinMode(FLAME_DO, INPUT);
  pinMode(PIR_PIN, INPUT);
  analogReadResolution(12);
  dht.begin();
  Serial.println("Sensor wiring test: AO should vary; DHT should show real values.");
}

void loop() {
  int ao = analogRead(FLAME_AO);
  int digitalFlame = digitalRead(FLAME_DO);
  int pir = digitalRead(PIR_PIN);
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  Serial.printf("flame AO=%d DO=%d | PIR=%d | DHT=", ao, digitalFlame, pir);
  if (isnan(t) || isnan(h)) Serial.println("READ FAILED");
  else Serial.printf("%.1f C, %.0f %%\n", t, h);
  delay(2000);
}
