#include <WiFi.h>
#include <Wire.h>
#include <SH1106Wire.h>
#include "time.h"
#include <HTTPClient.h>

#include "wifi_credentials.h"

// ---------------------- CONFIG ----------------------
// or use the wifi_credentials.h file and write the credentials there
// const char* ssid = "Wifi-SSID";
// const char* password = "Wifi-PW";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

#define SDA_PIN 9
#define SCL_PIN 8
#define OLED_ADDR 0x3C
SH1106Wire display(OLED_ADDR, SDA_PIN, SCL_PIN);

#define TOUCH_PIN 4

// ---------------------- MODE CONTROL ----------------------
int displayMode = 0; // 0 = idle eyes, 1 = time, 2 = BTC, 3 = credits
unsigned long modeStartTime = 0;
const unsigned long modeDuration = 20000; // 20 seconds

// ---------------------- TOUCH DEBOUNCE ----------------------
unsigned long lastTouchTime = 0;
const unsigned long debounceDelay = 400; // milliseconds

// ---------------------- EYE ANIMATION ----------------------
float leftEyeX = 40.0;
float rightEyeX = 80.0;
const int eyeY = 18;
const int eyeWidth = 25;
const int eyeHeight = 40;

float targetLeftEyeX = 40.0;
float targetRightEyeX = 80.0;
const float moveSpeed = 0.1;

int blinkState = 0;
int blinkDelay = 4000;
unsigned long lastBlinkTime = 0;
unsigned long moveTime = 0;

// ================================================================
//                        HELPER FUNCTIONS
// ================================================================

String getLocalTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "Time error";
  char timeString[100];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);
  return String(timeString);
}

bool timeSynced() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return false;
  return (timeinfo.tm_year > (1970 - 1900));
}

void startWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  display.clear();
  display.drawString(0, 0, "Connecting WiFi...");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  display.clear();
  display.drawString(0, 0, "WiFi connected");
  display.display();
  delay(1000);
}

void setupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  display.clear();
  display.drawString(0, 0, "Syncing time...");
  display.display();

  int retries = 0;
  while (!timeSynced() && retries < 20) {
    delay(1000);
    retries++;
  }

  display.clear();
  if (timeSynced())
    display.drawString(0, 0, "Time synced!");
  else
    display.drawString(0, 0, "Time sync failed!");
  display.display();
  delay(1500);
}

// ================================================================
//                        DISPLAY FUNCTIONS
// ================================================================

void showTime() {
  display.clear();
  display.setFont(ArialMT_Plain_24);
  String timeStr = getLocalTimeString();
  int w = display.getStringWidth(timeStr);
  int x = (128 - w) / 2;
  int y = (64 - 24) / 2;
  display.drawString(x, y, timeStr);
  display.display();
  display.setFont(ArialMT_Plain_16);
}

String getBtcPrice() {
    return "111111$";
}

void showBtc() {
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "BTC Price:");
  String btc = getBtcPrice();
  display.setFont(ArialMT_Plain_24);
  int w = display.getStringWidth(btc);
  int x = (128 - w) / 2;
  int y = (64 - 24) / 2;
  display.drawString(x, y, btc);
  display.display();
  display.setFont(ArialMT_Plain_16);
}

void showCredits() {
  display.clear();
  display.setFont(ArialMT_Plain_16);
  String msg = "Made by LennyE";
  int w = display.getStringWidth(msg);
  int x = (128 - w) / 2;
  int y = (64 - 16) / 2;
  display.drawString(x, y, msg);
  display.display();
}

// ================================================================
//                        EYE ANIMATION
// ================================================================

void blinkEyes() {
  unsigned long currentTime = millis();

  if (currentTime - lastBlinkTime > blinkDelay && blinkState == 0) {
    blinkState = 1;
    lastBlinkTime = currentTime;
  } else if (currentTime - lastBlinkTime > 400 && blinkState == 1) {
    blinkState = 0;
    lastBlinkTime = currentTime;
  }

  if (currentTime - moveTime > random(2000, 5000) && blinkState == 0) {
    int eyeMovement = random(0, 3);
    if (eyeMovement == 1) { targetLeftEyeX = 30; targetRightEyeX = 60; }
    else if (eyeMovement == 2) { targetLeftEyeX = 50; targetRightEyeX = 80; }
    else { targetLeftEyeX = 40; targetRightEyeX = 70; }
    moveTime = currentTime;
  }

  leftEyeX += (targetLeftEyeX - leftEyeX) * moveSpeed;
  rightEyeX += (targetRightEyeX - rightEyeX) * moveSpeed;

  display.clear();

  if (blinkState == 0) {
    display.fillRect((int)leftEyeX, eyeY, eyeWidth, eyeHeight);
    display.fillRect((int)rightEyeX, eyeY, eyeWidth, eyeHeight);
  } else {
    display.fillRect((int)leftEyeX, eyeY + eyeHeight / 2 - 2, eyeWidth, 4);
    display.fillRect((int)rightEyeX, eyeY + eyeHeight / 2 - 2, eyeWidth, 4);
  }

  display.display();
  delay(20);
}

// ================================================================
//                        SETUP & LOOP
// ================================================================

void setup() {
  Serial.begin(115200);
  pinMode(TOUCH_PIN, INPUT);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);

  startWifi();
  setupTime();

  display.clear();
  display.drawString(0, 0, "Ready");
  display.display();

  randomSeed(analogRead(A0));
}
void loop() {
  unsigned long now = millis();

  // Handle touch press with debounce
  if (digitalRead(TOUCH_PIN) == HIGH && now - lastTouchTime > debounceDelay) {
    lastTouchTime = now;

    // Only change mode when in eyes (idle) mode
    if (displayMode == 0) {
      static int nextInfoMode = 1; // remembers which info mode is next

      displayMode = nextInfoMode;
      modeStartTime = now;

      nextInfoMode++;
      if (nextInfoMode > 3) nextInfoMode = 1; // cycle 1→2→3→1

      Serial.printf("Mode changed to %d\n", displayMode);
    }

    delay(50); // small debounce delay
  }

  // MODE HANDLER
  switch (displayMode) {
    case 0: // idle eyes
      blinkEyes();
      break;

    case 1: // show time
      showTime();
      if (now - modeStartTime > modeDuration) displayMode = 0;
      delay(1000);
      break;

    case 2: // show BTC
      showBtc();
      if (now - modeStartTime > modeDuration) displayMode = 0;
      delay(5000);
      break;

    case 3: // show credits
      showCredits();
      if (now - modeStartTime > modeDuration) displayMode = 0;
      delay(1000);
      break;
  }
}
