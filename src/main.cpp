#include <WiFi.h>
#include <Wire.h>
#include <SH1106Wire.h>
#include "time.h"
#include <HTTPClient.h>
#include "wifi_credentials.h"

// ---------------------- CONFIG ----------------------
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
const unsigned long debounceDelay = 400;

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

// ---------------------- RANDOM VISUALS ----------------------
const int ROWS = 8;
const int COLS = 16;
uint8_t field[ROWS][COLS];
int blockWidth  = 128 / COLS;
int blockHeight = 64 / ROWS;

unsigned long lastAction = 0;
const unsigned long interval = 120000; // every 2 minutes
bool runningSpecial = false;

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

String getBtcPrice() {
  HTTPClient http;
  String url = "https://pro-api.coinmarketcap.com/v1/cryptocurrency/quotes/latest?symbol=BTC&convert=USD";
  http.begin(url);
  http.addHeader("Accepts", "application/json");
  http.addHeader("X-CMC_PRO_API_KEY", "e5e4719c13c842d982a8ed7a1da6d87b");

  int httpCode = http.GET();
  String payload;

  if (httpCode > 0) {
    payload = http.getString();

    int priceIndex = payload.indexOf("\"price\":");
    if (priceIndex > 0) {
      String sub = payload.substring(priceIndex + 8);
      int endIndex = sub.indexOf(",");
      if (endIndex > 0) {
        String priceStr = sub.substring(0, endIndex);
        float price = priceStr.toFloat();
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.2f$", price);
        http.end();
        return String(buffer);
      }
    }

    http.end();
    return "Parse error";
  } else {
    http.end();
    return "HTTP error";
  }
}

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

void showBtc() {
  display.clear();

  display.setFont(ArialMT_Plain_16);
  String title = "BTC Price";
  int titleWidth = display.getStringWidth(title);
  int titleX = (128 - titleWidth) / 2;
  display.drawString(titleX, 0, title);

  String btc = getBtcPrice();
  display.setFont(ArialMT_Plain_24);
  int priceWidth = display.getStringWidth(btc);
  int priceHeight = 24;
  int priceX = (128 - priceWidth) / 2;
  int priceY = (64 - priceHeight) / 2 + 8;
  display.drawString(priceX, priceY, btc);

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
//                        RANDOM VISUAL EFFECTS
// ================================================================

void clearField() {
  memset(field, 0, sizeof(field));
}

void drawField() {
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      if (field[y][x] == 1) {
        display.fillRect(x * blockWidth, y * blockHeight, blockWidth, blockHeight);
      }
    }
  }
  display.display();
}

bool isFull() {
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      if (field[y][x] == 0) return false;
    }
  }
  return true;
}

void addRandomBlock() {
  int r = random(ROWS);
  int c = random(COLS);
  field[r][c] = 1;
}

void runRandomTetris() {
  clearField();
  while (!isFull()) {
    addRandomBlock();
    drawField();
    delay(50);
  }
  delay(500);
  display.clear();
  display.display();
}

void runRandomBlocks(unsigned long startTime) {
  while (millis() - startTime < 10000) {
    display.clear();
    for (int y = 0; y < ROWS; y++) {
      for (int x = 0; x < COLS; x++) {
        if (random(0, 2) == 1) {
          display.fillRect(x * blockWidth, y * blockHeight, blockWidth, blockHeight);
        }
      }
    }
    display.display();
    delay(100);
  }
}

void runHackingRain(unsigned long startTime) {
  const int colWidth = 8;
  const int cols = 128 / colWidth;
  int yPositions[cols];

  for (int i = 0; i < cols; i++) {
    yPositions[i] = random(-64, 0);
  }

  while (millis() - startTime < 10000) {
    display.clear();
    for (int i = 0; i < cols; i++) {
      int y = yPositions[i];
      int x = i * colWidth;
      char c = random(2) ? '0' : '1';
      display.drawString(x, y, String(c));
      yPositions[i] += 8;
      if (yPositions[i] > 64) {
        yPositions[i] = random(-20, 0);
      }
    }
    display.display();
    delay(80);
  }

  display.clear();
  display.display();
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

    if (displayMode == 0) {
      static int nextInfoMode = 1;
      displayMode = nextInfoMode;
      modeStartTime = now;

      nextInfoMode++;
      if (nextInfoMode > 3) nextInfoMode = 1;

      Serial.printf("Mode changed to %d\n", displayMode);
    }

    delay(50);
  }

  switch (displayMode) {
    case 0: blinkEyes(); break;
    case 1: showTime(); if (now - modeStartTime > modeDuration) displayMode = 0; delay(1000); break;
    case 2: showBtc();  if (now - modeStartTime > modeDuration) displayMode = 0; delay(5000); break;
    case 3: showCredits(); if (now - modeStartTime > modeDuration) displayMode = 0; delay(1000); break;
  }

  unsigned long currentMillis = millis();
  if (!runningSpecial && (currentMillis - lastAction >= interval)) {
    lastAction = currentMillis;
    runningSpecial = true;

    int choice = random(6); // 0–5 → 50% chance nothing happens
    if (choice == 0) runRandomTetris();
    else if (choice == 1) runRandomBlocks(currentMillis);
    else if (choice == 2) runHackingRain(currentMillis);

    runningSpecial = false;
  }
}
