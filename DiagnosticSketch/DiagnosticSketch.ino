#include <Wire.h>
#include "SSD1306Wire.h"
#include "HX711.h"
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>

/*
  Smart Ballot Box Scale Firmware – Diagnostic Version (OLED On)

  Weight measurements and Tier updates are realtime and displayed on the OLED
  for easy testing and demonstration purposes. If this program is left on it
  will work for deployment but the battery will run out in just a few days. 
  For true deployment of the scale the deployment sketch is recommended.
*/

// OLED
SSD1306Wire display(0x3c, SDA_OLED, SCL_OLED);

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

void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void displayReset() {
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, HIGH); delay(1);
  digitalWrite(RST_OLED, LOW); delay(1);
  digitalWrite(RST_OLED, HIGH); delay(1);
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
  VextON();
  displayReset();

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);

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
  static bool diagPressed = false;
  unsigned long now = millis();

  bool tareReading = digitalRead(TARE_BUTTON_PIN);
  if (tareReading == LOW && !tarePressed && (now - lastDebounceTime > debounceDelay)) {
    scale.tare();
    tareOffset = scale.get_offset();
    saveTareOffset(tareOffset);
    currentTier = 0;
    lastTierChangeTime = now;
    tarePressed = true;
  }
  if (tareReading == HIGH) tarePressed = false;

  float raw = scale.get_units(1);
  sum -= readings[bufferIndex];
  readings[bufferIndex] = raw;
  sum += raw;
  bufferIndex = (bufferIndex + 1) % averageWindow;
  float smoothedGrams = (sum / averageWindow) * scaleFactor;
  float smoothedKg = smoothedGrams / 1000.0;
  float lbs = smoothedKg * 2.2046;

  float lowerBound = (currentTier * tierStepLbs) - hysteresisMargin;
  float upperBound = ((currentTier + 1) * tierStepLbs) + hysteresisMargin;
  if (lbs > upperBound) {
    currentTier++;
    lastTierChangeTime = now;
  } else if (lbs < lowerBound && currentTier > 0) {
    currentTier--;
    lastTierChangeTime = now;
  }

  float vbat = readBatteryVoltage();
  int vpercent = voltageToPercent(vbat);

  uint8_t frameVersion = FRAME_VERSION_INFO;
  if (now - lastTierChangeTime < stateFrameDuration) {
    frameVersion = FRAME_VERSION_STATE;
  }

  updateBLEAdvertisement(frameVersion, vpercent, currentTier);

  display.clear();
  display.drawString(64, 0, "Weight: " + String(lbs, 1) + " lbs");
  display.drawString(64, 16, "Battery: " + String(vbat, 2) + " V / " + String(vpercent) + "%");
  display.drawString(64, 28, "Tier: " + String(currentTier) + " [" + (frameVersion == FRAME_VERSION_STATE ? "State" : "Info") + "]");
  display.display();

  delay(1000);
}

