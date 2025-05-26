#include <Wire.h>
#include "HX711.h"
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>

/*
  Smart Ballot Box Scale Firmware – Deployment Version (OLED off)

  This program shows nothing on the OLED display and will only take weight
  measurements on a set interval (see measurementInterval variable below).
  This interval saves battery, and if needed it can be changed to trade off
  responsiveness and battery life as desired. When testing keep in mind that
  the scale is programmed to only change by one tier maximum during each
  measurement. This means that if the measurement interval is 1 minute, it will
  take 3 minutes (3 intervals) before the scale can update all the way from
  tier 0 to tier 3 (0-1, 1-2, 2-3).
*/

// HX711
#define HX711_DOUT 3
#define HX711_SCK 2
HX711 scale;

// Calibration
float scaleFactor = 0.2725;
const int averageWindow = 5;
float readings[averageWindow];
int bufferIndex = 0;
float sum = 0;

// Battery sensing
#define VBAT_PIN 1
#define VBAT_CTRL_PIN 37
const float DIVIDER_RATIO = 100.0 / (100.0 + 450.0);
const float VBAT_CALIBRATION = 0.975;

// Buttons
#define TARE_BUTTON_PIN 4

// Tare state
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
long tareOffset = 0;
#define EEPROM_OFFSET_ADDR 0

// BLE
BLEAdvertising* pAdvertising;
#define MANUFACTURER_ID 0x0A1A // LinkLabs ID
#define FRAME_TYPE 0xCA
uint8_t FRAME_VERSION_INFO = 0x00;
uint8_t FRAME_VERSION_STATE = 0x03;
BLEAdvertisementData advertData;

// Tier tracking
int currentTier = 0;
int lastSentTier = -1;
unsigned long lastTierChangeTime = 0;
const unsigned long stateFrameDuration = 15UL * 60UL * 1000UL; // 15 min
const float tierStepLbs = 50.0;
const float hysteresisMargin = 5.0;

// Weight Measurement Interval (Tradeoff responsiveness for battery life)
unsigned long lastMeasurementTime = 0;
const unsigned long measurementInterval = 5UL * 60UL * 1000UL; // 5 minutes
// const unsigned long measurementInterval = 1UL * 60UL * 1000UL; // 1 minute for testing
bool scaleInitialized = false;

void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

float readBatteryVoltage() {
  int raw = analogRead(VBAT_PIN);
  float v_adc = raw / 4095.0 * 3.3;
  return (v_adc / DIVIDER_RATIO) * VBAT_CALIBRATION;
}

int voltageToPercent(float v) {
  if (v >= 4.2) return 100;
  if (v <= 3.0) return 0;
  return (int)((v - 3.0) / (4.2 - 3.0) * 100);
}

void saveTareOffset(long offset) {
  EEPROM.put(EEPROM_OFFSET_ADDR, offset);
  EEPROM.commit();
}

long loadTareOffset() {
  long offset;
  EEPROM.get(EEPROM_OFFSET_ADDR, offset);
  return offset;
}

void updateBLEAdvertisement(uint8_t frameVersion, int batteryPercent, int tier) {
  BLEAdvertisementData data;

  // --- Flags AD Structure ---
  uint8_t flags[] = {
    0x02, // Length
    0x01, // Type: Flags
    0x06  // LE General Discoverable Mode | BR/EDR Not Supported
  };

  String flagData;
  flagData.reserve(sizeof(flags));
  for (int i = 0; i < sizeof(flags); i++) {
    flagData += (char)flags[i];
  }
  data.addData(flagData);

  // --- Manufacturer Specific Data ---
  uint8_t mfgPayload[6];
  mfgPayload[0] = lowByte(MANUFACTURER_ID);
  mfgPayload[1] = highByte(MANUFACTURER_ID);
  mfgPayload[2] = FRAME_TYPE;
  mfgPayload[3] = frameVersion;
  mfgPayload[4] = batteryPercent;
  mfgPayload[5] = tier;

  String mfgData;
  mfgData.reserve(sizeof(mfgPayload));
  for (int i = 0; i < sizeof(mfgPayload); i++) {
    mfgData += (char)mfgPayload[i];
  }
  data.setManufacturerData(mfgData);

  // --- Set & Start Advertising ---
  pAdvertising->setAdvertisementData(data);
  pAdvertising->start();
}


void setup() {
  Serial.begin(115200);
  EEPROM.begin(64);
  // VextON();

  pinMode(VBAT_CTRL_PIN, OUTPUT);
  digitalWrite(VBAT_CTRL_PIN, HIGH);
  pinMode(TARE_BUTTON_PIN, INPUT_PULLUP);

  scale.begin(HX711_DOUT, HX711_SCK);
  delay(100);
  tareOffset = loadTareOffset();
  scale.set_offset(tareOffset);

  for (int i = 0; i < averageWindow; i++) readings[i] = 0;

  BLEDevice::init("BallotBoxScale");
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void loop() {
  static bool tarePressed = false;
  unsigned long now = millis();

  // Handle tare button
  bool tareReading = digitalRead(TARE_BUTTON_PIN);
  if (tareReading == LOW && !tarePressed && (now - lastDebounceTime > debounceDelay)) {
    VextON();
    delay(100);
    scale.begin(HX711_DOUT, HX711_SCK);
    delay(100);
    scale.tare();
    tareOffset = scale.get_offset();
    saveTareOffset(tareOffset);
    currentTier = 0;
    lastTierChangeTime = now;
    tarePressed = true;
    VextOFF();
  }
  if (tareReading == HIGH) tarePressed = false;

  // Perform weight reading every 5 minutes
  if (now - lastMeasurementTime >= measurementInterval || !scaleInitialized) {
    lastMeasurementTime = now;

    // Power up sensors
    VextON();
    delay(100); // allow voltage rail to settle
    scale.begin(HX711_DOUT, HX711_SCK);
    delay(100);
    scale.set_offset(tareOffset);

    // Take smoothed weight reading
    sum = 0;
    for (int i = 0; i < averageWindow; i++) {
      readings[i] = scale.get_units(1);
      sum += readings[i];
      delay(10);
    }
    float smoothedGrams = (sum / averageWindow) * scaleFactor;
    float smoothedKg = smoothedGrams / 1000.0;
    float lbs = smoothedKg * 2.2046;

    // Update tier
    float lowerBound = (currentTier * tierStepLbs) - hysteresisMargin;
    float upperBound = ((currentTier + 1) * tierStepLbs) + hysteresisMargin;
    if (lbs > upperBound) {
      currentTier++;
      lastTierChangeTime = now;
    } else if (lbs < lowerBound && currentTier > 0) {
      currentTier--;
      lastTierChangeTime = now;
    }

    // Battery and BLE
    float vbat = readBatteryVoltage();
    int vpercent = voltageToPercent(vbat);
    uint8_t frameVersion = (now - lastTierChangeTime < stateFrameDuration) ? FRAME_VERSION_STATE : FRAME_VERSION_INFO;

    updateBLEAdvertisement(frameVersion, vpercent, currentTier);

    scaleInitialized = true;

    // Power down sensors
    VextOFF();
  }

  delay(1000); // Run loop slowly while waiting
}
