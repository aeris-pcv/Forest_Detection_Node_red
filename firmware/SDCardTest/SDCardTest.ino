/*
  microSD wiring and read/write test for DOIT ESP32 DEVKIT V1.
  Uses the same pins as ForestFireIoT. No Wi-Fi or sensors required.

  SD module VCC  -> 3V3 (only if the module is rated for 3.3 V)
  SD module GND  -> ESP32 GND
  SD module SCK  -> GPIO18
  SD module MISO -> GPIO19
  SD module MOSI -> GPIO23
  SD module CS   -> GPIO22

  Format the card FAT32. Open Serial Monitor at 115200 baud.
  This sketch only appends to /sd_test.txt; it does not erase other files.
*/

#include <SPI.h>
#include <SD.h>

constexpr int SD_CS = 22;
constexpr int SD_SCK = 18;
constexpr int SD_MISO = 19;
constexpr int SD_MOSI = 23;
constexpr uint32_t SD_CLOCK_HZ = 1000000;  // start slowly for reliable wiring check
const char* TEST_FILE = "/sd_test.txt";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32 microSD test ===");

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI, SD_CLOCK_HZ)) {
    Serial.println("FAIL: SD.begin could not initialize the card.");
    Serial.println("Check VCC/GND, CS=22, SCK=18, MISO=19, MOSI=23, and FAT32 format.");
    return;
  }

  const uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("FAIL: no card detected in the SD reader.");
    return;
  }
  Serial.print("Card type: ");
  if (cardType == CARD_MMC) Serial.println("MMC");
  else if (cardType == CARD_SD) Serial.println("SD");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");
  else Serial.println("unknown");
  Serial.printf("Card size: %llu MB\n", SD.cardSize() / (1024ULL * 1024ULL));

  char testLine[64];
  snprintf(testLine, sizeof(testLine), "ESP32 SD write/read test at uptime %lu ms", millis());

  File file = SD.open(TEST_FILE, FILE_APPEND);
  if (!file) {
    Serial.println("FAIL: could not open /sd_test.txt for writing.");
    return;
  }
  if (!file.println(testLine)) {
    Serial.println("FAIL: write did not complete.");
    file.close();
    return;
  }
  file.close();
  Serial.print("Wrote: ");
  Serial.println(testLine);

  file = SD.open(TEST_FILE, FILE_READ);
  if (!file) {
    Serial.println("FAIL: could not reopen /sd_test.txt for reading.");
    return;
  }
  String lastLine;
  while (file.available()) {
    String candidate = file.readStringUntil('\n');
    candidate.trim();
    if (candidate.length()) lastLine = candidate;
  }
  file.close();

  Serial.print("Read back: ");
  Serial.println(lastLine);
  if (lastLine == testLine) Serial.println("PASS: SD card write and read both work.");
  else Serial.println("FAIL: read-back text did not match the written text.");
}

void loop() {
  // One test per boot, so the card is not filled by repeated writes.
}
