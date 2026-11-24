/*
  Forest Fire Detection & Alert System  -  v5 (flame false-alarm fix)
  Board : ESP32 DevKit V1 (DOIT)

  Changes from v4
    - Flame detection uses the ANALOG pin only by default (USE_FLAME_DO = false).
      The digital pin (DO) was reading LOW all the time and caused a permanent ALARM.
    - Optional DEBUG_SENSORS prints AO / DO values to Serial for calibration.
    - Comments updated: power saving = CPU scaling + delay(), not light sleep.
    - Removed unused variables (alarmLatched, tServoDone).

  Requirements covered
    1. Push switch : long press (1.5 s) = system ON / OFF (low-power idle),
                     short press = mute / unmute, double press = battery saver
    2. Data        : TFT live display + microSD CSV log (+ Serial print)
    3. Timestamps  : NTP over WiFi (UTC+7). Clock keeps running while OFF
    4. Technique   : Finite State Machine (OFF / SAFE / WARNING / ALARM)
                     + external interrupt on the button
    5. Servo       : points to a different angle for every state

  Libraries (Library Manager):
    Adafruit GFX Library, Adafruit ILI9341, Adafruit BusIO,
    DHT sensor library, Adafruit Unified Sensor, ESP32Servo
    (SD, SPI, WiFi come with the ESP32 core)
*/

#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <DHT.h>
#include <ESP32Servo.h>

// ---------------- USER SETTINGS ----------------
const char* WIFI_SSID = "wifi-name";      // remove before sharing / submitting the code
const char* WIFI_PASS = "password";        // remove before sharing / submitting the code
const long  GMT_OFFSET_SEC = 7 * 3600;      // Bangkok = UTC+7
const int   DST_OFFSET_SEC = 0;
#define USE_SD true                          // set false if you have no SD module
#define USE_FLAME_DO false                   // true = also use the flame module's digital pin (DO)
#define DEBUG_SENSORS false                  // true = print AO / DO values to Serial

// ---------------- Servo settings ----------------
// A normal SG90 / MG90S servo only turns 0-180 degrees, so 270 and 360 are
// impossible. Your 90/180/270/360 plan is therefore scaled by 1/2.
// Choose ONE option:
//
// OPTION A (standard 180-degree servo)  <-- default
const int SERVO_RANGE_DEG = 180;
const int ANGLE_SAFE  = 45;     // = "90"  in your plan
const int ANGLE_WARN  = 90;     // = "180"
const int ANGLE_ALARM = 135;    // = "270"
const int ANGLE_OFF   = 180;    // = "360"
//
// OPTION B (270-degree servo, e.g. some DS3218/25kg types):
//   SERVO_RANGE_DEG = 270; SAFE = 90; WARN = 180; ALARM = 270; OFF = 0
//   (a servo can never reach 360, so OFF uses 0 = the start position)
//
// A 360-degree "continuous rotation" servo can NOT hold angles - don't use it.

const int SERVO_MIN_US = 500;    // pulse width at 0 deg
const int SERVO_MAX_US = 2400;   // pulse width at SERVO_RANGE_DEG
const int SERVO_STEP_DEG = 3;    // smooth movement: degrees per step
const unsigned long SERVO_STEP_MS = 15;

// ---------------- Pin map ----------------
#define TFT_CS      5
#define TFT_DC      16      // module label: DC or RS   (RX2)
#define TFT_RST     17      // module label: RESET      (TX2)
                            // TFT SDI/MOSI = GPIO23, SCK = GPIO18, LED = 3V3
                            // TFT SDO/MISO: leave unconnected
#define SD_CS       22      // SD shares SCK 18 / MOSI 23 / MISO 19 with the TFT
#define DHT_PIN     4
#define DHT_TYPE    DHT11
#define FLAME_AO    34
#define FLAME_DO    35
#define PIR_PIN     27
#define BTN_PIN     13      // button to GND, internal pull-up
#define BUZZER_PIN  26      // active buzzer
#define LED_R       25
#define LED_G       33
#define LED_B       32
#define SERVO_PIN   21

// ---------------- Thresholds ----------------
const float TEMP_WARN        = 38.0;
const float TEMP_ALARM       = 45.0;
const float TEMP_DRY_WARN    = 33.0;
const float HUM_DRY          = 30.0;
const int   FLAME_ANALOG_MAX = 2000;         // AO below this = flame. Calibrate!
const unsigned long PIR_WARMUP_MS    = 30000;
const unsigned long LONG_PRESS_MS    = 1500;
const unsigned long LOG_INTERVAL_MS  = 5000;
const unsigned long RESYNC_MS        = 3600000UL;   // re-sync NTP on power-on if older than 1 h
const char* LOG_FILE = "/fire_log.csv";
const unsigned long ALARM_CLEAR_MS  = 3000;    // danger must be gone this long before ALARM -> SAFE (0 = instant)
const unsigned long DOUBLE_PRESS_MS = 400;     // 2 short presses within this = battery-saver toggle

// Normal vs battery-saver timing (battery-saver only slows things down while SAFE)
const unsigned long NORM_DHT_MS = 2000, NORM_SENSOR_MS = 100, NORM_DISPLAY_MS = 400;
const unsigned long ECO_DHT_MS  = 5000, ECO_SENSOR_MS  = 300, ECO_DISPLAY_MS  = 1500, ECO_LOG_MS = 30000;
const unsigned long ECO_NAP_MS  = 100;         // idle delay between loops in battery-saver

// ---------------- Objects ----------------
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
DHT dht(DHT_PIN, DHT_TYPE);
Servo stateServo;

// ---------------- Finite state machine ----------------
enum SysState { SYS_OFF, SYS_SAFE, SYS_WARNING, SYS_ALARM };
SysState sys = SYS_SAFE;
SysState lastDrawn = (SysState)255;

// Button event type. It must be declared up here, before the first function,
// because the Arduino IDE auto-generates function prototypes at that point.
enum BtnEvent { BTN_NONE, BTN_SHORT, BTN_DOUBLE, BTN_LONG };

const char* stateName(SysState s) {
  switch (s) {
    case SYS_OFF:     return "OFF";
    case SYS_SAFE:    return "SAFE";
    case SYS_WARNING: return "WARNING";
    default:          return "ALARM";
  }
}

int servoAngleFor(SysState s) {
  switch (s) {
    case SYS_OFF:     return ANGLE_OFF;
    case SYS_SAFE:    return ANGLE_SAFE;
    case SYS_WARNING: return ANGLE_WARN;
    default:          return ANGLE_ALARM;
  }
}

// ---------------- Data ----------------
float temperature = 0, humidity = 0;
bool  dhtOk = false;
int   flameAnalog = 4095;
bool  flame = false, motion = false;
bool  muted = false;
unsigned long lastDangerAt = 0;
bool  batteryMode = false;                  // false = normal, true = battery saver
bool  servoAttached = false;
int   servoAngle = ANGLE_OFF;               // current position (starts at OFF position)

bool sdOk = false;
bool timeValid = false;
unsigned long lastSyncMs = 0;

unsigned long tDht = 0, tSensors = 0, tDisplay = 0, tServo = 0, tLog = 0;

// ---------------- Button (interrupt driven) ----------------
volatile bool btnFlag = false;
volatile unsigned long btnIsrTime = 0;
bool btnTracking = false, btnLongFired = false;
bool pendingShort = false;                // waiting to see if a 2nd short press follows
unsigned long pendingShortAt = 0;
unsigned long btnDownAt = 0;

void IRAM_ATTR onButtonISR() {            // keep ISR tiny: just raise a flag
  unsigned long t = millis();
  if (t - btnIsrTime > 60) {              // simple debounce
    btnIsrTime = t;
    btnFlag = true;
  }
}

BtnEvent pollButton() {
  if (btnFlag && !btnTracking) {
    btnFlag = false;
    btnTracking = true;
    btnLongFired = false;
    btnDownAt = millis();
  }
  if (!btnTracking) {
    if (pendingShort && millis() - pendingShortAt > DOUBLE_PRESS_MS) {   // no 2nd press -> single
      pendingShort = false;
      return BTN_SHORT;
    }
    return BTN_NONE;
  }

  bool down = (digitalRead(BTN_PIN) == LOW);
  unsigned long held = millis() - btnDownAt;

  if (down && !btnLongFired && held >= LONG_PRESS_MS) {
    btnLongFired = true;
    pendingShort = false;
    return BTN_LONG;
  }
  if (!down) {
    btnTracking = false;
    btnFlag = false;
    if (!btnLongFired && held >= 30) {
      if (pendingShort && millis() - pendingShortAt <= DOUBLE_PRESS_MS) {
        pendingShort = false;
        return BTN_DOUBLE;
      }
      pendingShort = true;
      pendingShortAt = millis();
    }
  }
  return BTN_NONE;
}

// ---------------- Time (NTP) ----------------
bool syncTime() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(250);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, "pool.ntp.org", "time.google.com");
    struct tm ti;
    if (getLocalTime(&ti, 8000)) {
      timeValid = true;
      lastSyncMs = millis();
      Serial.println("NTP time set");
    } else {
      Serial.println("NTP failed (no internet?)");
    }
  } else {
    Serial.println("WiFi failed (check name, password, 2.4 GHz)");
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);                    // save power, clock keeps running internally
  return timeValid;
}

void timestamp(char* buf, size_t n) {
  time_t now = time(nullptr);
  if (timeValid && now > 1700000000) {
    struct tm ti;
    localtime_r(&now, &ti);
    strftime(buf, n, "%Y-%m-%d %H:%M:%S", &ti);
  } else {
    snprintf(buf, n, "uptime_%lus", millis() / 1000);
  }
}

// ---------------- Logging ----------------
void initSD() {
#if USE_SD
  if (SD.begin(SD_CS)) {
    sdOk = true;
    if (!SD.exists(LOG_FILE)) {
      File f = SD.open(LOG_FILE, FILE_WRITE);
      if (f) {
        f.println("timestamp,event,state,temp_c,humidity,flame_ao,flame,motion,servo_deg");
        f.close();
      }
    }
  } else {
    Serial.println("SD.begin FAILED - check wiring (MISO = GPIO 19), power, FAT32");
  }
#endif
}

void logRow(const char* event) {
  char ts[24];
  timestamp(ts, sizeof(ts));
  char line[160];
  snprintf(line, sizeof(line), "%s,%s,%s,%.1f,%.0f,%d,%d,%d,%d",
           ts, event, stateName(sys), temperature, humidity,
           flameAnalog, flame, motion, servoAngle);
  Serial.println(line);
#if USE_SD
  if (sdOk) {
    File f = SD.open(LOG_FILE, FILE_APPEND);
    if (f) { f.println(line); f.close(); }
    else   { sdOk = false; }
  }
#endif
}

// ---------------- Outputs ----------------
void setLED(bool r, bool g, bool b) {
  digitalWrite(LED_R, r);                 // common cathode; invert for common anode
  digitalWrite(LED_G, g);
  digitalWrite(LED_B, b);
}

void updateOutputs() {
  if (sys == SYS_OFF) {
    setLED(0, 0, 0);
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }
  if (sys == SYS_SAFE)         setLED(0, batteryMode ? ((millis() % 2000) < 150) : 1, 0);
  else if (sys == SYS_WARNING) setLED(1, 1, 0);
  else                         setLED(1, 0, 0);                 // ALARM = solid red

  if (muted || sys == SYS_SAFE)      digitalWrite(BUZZER_PIN, LOW);
  else if (sys == SYS_WARNING)       digitalWrite(BUZZER_PIN, (millis() % 3000) < 80);
  else                               digitalWrite(BUZZER_PIN, (millis() % 300) < 150);
}

// ---------------- Servo (state pointer) ----------------
void servoWriteDeg(int deg) {
  deg = constrain(deg, 0, SERVO_RANGE_DEG);
  int us = map(deg, 0, SERVO_RANGE_DEG, SERVO_MIN_US, SERVO_MAX_US);
  stateServo.writeMicroseconds(us);
}

void servoAttachIfNeeded() {
  if (!servoAttached) {
    stateServo.setPeriodHertz(50);
    stateServo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
    servoAttached = true;
    servoWriteDeg(servoAngle);
  }
}

void servoRelease() {                       // no PWM = no servo current
  if (servoAttached) { stateServo.detach(); servoAttached = false; }
}

// Moves one small step toward the angle that belongs to the current state.
void updateServo() {
  int target = servoAngleFor(sys);
  if (servoAngle == target) {
    return;
  }
  servoAttachIfNeeded();
  if (millis() - tServo < SERVO_STEP_MS) return;
  tServo = millis();
  if (servoAngle < target) servoAngle = min(servoAngle + SERVO_STEP_DEG, target);
  else                     servoAngle = max(servoAngle - SERVO_STEP_DEG, target);
  servoWriteDeg(servoAngle);
}

// ---------------- Display (320 x 240 landscape) ----------------
uint16_t stateColor() {
  if (sys == SYS_ALARM)   return ILI9341_RED;
  if (sys == SYS_WARNING) return ILI9341_YELLOW;
  return ILI9341_GREEN;
}

void drawHeader() {
  uint16_t c = stateColor();
  tft.fillRect(0, 0, 320, 36, c);
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_BLACK, c);
  tft.setCursor(10, 7);
  if (sys == SYS_ALARM)        tft.print("FIRE ALARM!");
  else if (sys == SYS_WARNING) tft.print("WARNING    ");
  else                         tft.print("SAFE       ");
  tft.setTextSize(2);                       // mode tag on the right side of the header
  tft.setCursor(256, 12);
  tft.print(batteryMode ? "BATT" : "    ");
}

void row(int y, const char* label, const char* value, uint16_t color) {
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setCursor(6, y);
  tft.print(label);
  char buf[24];
  snprintf(buf, sizeof(buf), "%-15s", value);     // pad to erase old text
  tft.setTextColor(color, ILI9341_BLACK);
  tft.setCursor(130, y);
  tft.print(buf);
}

void updateDisplay() {
  if (sys != lastDrawn) { drawHeader(); lastDrawn = sys; }
  char buf[28];

  if (dhtOk) snprintf(buf, sizeof(buf), "%.1f C", temperature);
  else       snprintf(buf, sizeof(buf), "sensor error");
  row(46, "Temp", buf, temperature >= TEMP_WARN ? ILI9341_ORANGE : ILI9341_WHITE);

  if (dhtOk) snprintf(buf, sizeof(buf), "%.0f %%", humidity);
  else       snprintf(buf, sizeof(buf), "--");
  row(70, "Humidity", buf, humidity <= HUM_DRY ? ILI9341_ORANGE : ILI9341_WHITE);

  snprintf(buf, sizeof(buf), "%d %s", flameAnalog, flame ? "FIRE!" : "clear");
  row(94, "Flame", buf, flame ? ILI9341_RED : ILI9341_GREEN);

  if (millis() < PIR_WARMUP_MS)           snprintf(buf, sizeof(buf), "warming up");
  else if (motion && sys == SYS_ALARM)    snprintf(buf, sizeof(buf), "PERSON NEAR!");
  else if (motion)                        snprintf(buf, sizeof(buf), "detected");
  else                                    snprintf(buf, sizeof(buf), "none");
  row(118, "Motion", buf, motion ? ILI9341_ORANGE : ILI9341_WHITE);

  snprintf(buf, sizeof(buf), "%d deg", servoAngle);
  row(142, "Servo", buf, ILI9341_WHITE);

  row(166, "Buzzer", muted ? "MUTED" : "armed", muted ? ILI9341_ORANGE : ILI9341_WHITE);

  row(190, "Log", sdOk ? "SD card ok" : "serial only", sdOk ? ILI9341_GREEN : ILI9341_ORANGE);

  char ts[24];
  timestamp(ts, sizeof(ts));
  snprintf(buf, sizeof(buf), "%-24s", ts);
  tft.setTextSize(2);
  tft.setTextColor(timeValid ? ILI9341_WHITE : ILI9341_ORANGE, ILI9341_BLACK);
  tft.setCursor(6, 216);
  tft.print(buf);
}

// ---------------- Sensors ----------------
void readSensors() {
  flameAnalog = analogRead(FLAME_AO);
  flame = (flameAnalog < FLAME_ANALOG_MAX);              // analog: lower value = more infrared light
#if USE_FLAME_DO
  flame = flame || (digitalRead(FLAME_DO) == LOW);       // most modules: LOW = flame
#endif
  motion = (digitalRead(PIR_PIN) == HIGH) && (millis() > PIR_WARMUP_MS);
#if DEBUG_SENSORS
  Serial.printf("AO=%d DO=%d PIR=%d\n", flameAnalog, digitalRead(FLAME_DO), digitalRead(PIR_PIN));
#endif
}

void readDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) { dhtOk = false; }
  else { temperature = t; humidity = h; dhtOk = true; }
}

// ---------------- Power on / off ----------------
void startSystem() {
  tft.sendCommand(ILI9341_DISPON);
  tft.fillScreen(ILI9341_BLACK);
  servoAttached = false;
  servoAttachIfNeeded();                  // attaches and writes the current angle
  servoWriteDeg(servoAngle);              // starts at the OFF position, then glides to SAFE
  readDHT();
  readSensors();
  muted = false;
  sys = SYS_SAFE;
  lastDrawn = (SysState)255;
  logRow("SYSTEM_ON");
}

void powerOn() {
  tft.sendCommand(ILI9341_DISPON);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(20, 110);
  tft.print("Starting...");
  if (!timeValid || millis() - lastSyncMs > RESYNC_MS) syncTime();
  startSystem();
}

void powerOff() {
  logRow("SYSTEM_OFF");
  setLED(0, 0, 0);
  digitalWrite(BUZZER_PIN, LOW);

  // glide the servo to the OFF position, then release it to save power
  servoAttachIfNeeded();
  while (servoAngle != ANGLE_OFF) {
    if (servoAngle < ANGLE_OFF) servoAngle = min(servoAngle + SERVO_STEP_DEG, ANGLE_OFF);
    else                        servoAngle = max(servoAngle - SERVO_STEP_DEG, ANGLE_OFF);
    servoWriteDeg(servoAngle);
    delay(SERVO_STEP_MS);
  }
  delay(300);
  servoRelease();

  tft.fillScreen(ILI9341_BLACK);
  tft.sendCommand(ILI9341_DISPOFF);
  sys = SYS_OFF;
}

// Power saving WITHOUT light sleep (light sleep can switch off the pins/PWM that the
// PIR, button and servo use). Instead we lower the CPU clock and idle with delay().
void applyCpuFreq() {
  uint32_t f = (sys == SYS_OFF || (batteryMode && sys == SYS_SAFE)) ? 80 : 240;
  if (getCpuFrequencyMhz() != f) setCpuFrequencyMhz(f);
}

void napMs(unsigned long ms) {
  if (!btnTracking && digitalRead(BTN_PIN) == LOW) { btnFlag = true; btnIsrTime = millis(); }  // catch a press
  delay(ms);
}

void sleepUntilButton() { napMs(50); }       // OFF state: check the button every 50 ms

// ---------------- FSM step ----------------
void stepFSM(BtnEvent ev) {
  bool danger = flame || (dhtOk && temperature >= TEMP_ALARM);
  bool warn   = dhtOk && (temperature >= TEMP_WARN ||
                         (humidity <= HUM_DRY && temperature >= TEMP_DRY_WARN));

  if (ev == BTN_SHORT) {
    muted = !muted;
    logRow(muted ? "MUTED" : "UNMUTED");
  }

  if (danger) lastDangerAt = millis();

  SysState next = sys;
  switch (sys) {
    case SYS_SAFE:
      if (danger)     next = SYS_ALARM;
      else if (warn)  next = SYS_WARNING;
      break;
    case SYS_WARNING:
      if (danger)     next = SYS_ALARM;
      else if (!warn) next = SYS_SAFE;
      break;
    case SYS_ALARM:
      // leave ALARM automatically once the flame/heat has been gone for ALARM_CLEAR_MS
      if (!danger && millis() - lastDangerAt >= ALARM_CLEAR_MS)
        next = warn ? SYS_WARNING : SYS_SAFE;
      break;
    default:
      break;
  }
  if (sys == SYS_ALARM && next != SYS_ALARM) muted = false;   // next alarm will sound again

  if (next != sys) {
    sys = next;
    logRow("STATE_CHANGE");
    tLog = millis();
  }
}

// ---------------- Arduino ----------------
void setup() {
  Serial.begin(115200);

  pinMode(FLAME_DO, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  analogReadResolution(12);
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), onButtonISR, FALLING);

  dht.begin();
  stateServo.setPeriodHertz(50);

  tft.begin();
  tft.setRotation(1);                 // landscape 320x240; try 3 if the image is upside down
  tft.fillScreen(ILI9341_BLACK);

  setLED(0, 0, 1);                    // blue = booting
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(20, 80);
  tft.print("Forest Fire Detector");
  tft.setCursor(20, 110);
  tft.print("Syncing clock (NTP)...");

  syncTime();
  initSD();
  startSystem();
}

bool eco() { return batteryMode && sys == SYS_SAFE; }   // slow + idle only while SAFE

void toggleBatteryMode() {
  batteryMode = !batteryMode;
  if (!batteryMode) servoAttachIfNeeded();
  lastDrawn = (SysState)255;                 // redraw header so the mode tag updates
  logRow(batteryMode ? "MODE_BATTERY_SAVER" : "MODE_NORMAL");
  tLog = millis();
}

void loop() {
  unsigned long now = millis();
  BtnEvent ev = pollButton();

  if (ev == BTN_LONG) {               // push switch: system ON / OFF
    if (sys == SYS_OFF) powerOn(); else powerOff();
    return;
  }

  applyCpuFreq();

  if (sys == SYS_OFF) {               // power-saving state
    sleepUntilButton();
    return;
  }

  if (ev == BTN_DOUBLE) { toggleBatteryMode(); ev = BTN_NONE; }

  bool slow = eco();
  if (now - tDht >= (slow ? ECO_DHT_MS : NORM_DHT_MS))           { tDht = now; readDHT(); }
  if (now - tSensors >= (slow ? ECO_SENSOR_MS : NORM_SENSOR_MS)) { tSensors = now; readSensors(); }

  stepFSM(ev);
  updateServo();
  updateOutputs();

  if (now - tDisplay >= (slow ? ECO_DISPLAY_MS : NORM_DISPLAY_MS)) { tDisplay = now; updateDisplay(); }
  if (now - tLog >= (slow ? ECO_LOG_MS : LOG_INTERVAL_MS))         { tLog = now; logRow("PERIODIC"); }

  if (eco()) napMs(ECO_NAP_MS);       // battery saver: idle between loops (80 MHz clock)
}
