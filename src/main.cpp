#include <Wire.h>
#include "SH1106Wire.h"

#define OLED_ADDR 0x3C
SH1106Wire display(OLED_ADDR, 8, 9);  // SDA = 8, SCL = 9

float leftEyeX = 40.0;
float rightEyeX = 80.0;
const int eyeY = 18;
const int eyeWidth =25;
const int eyeHeight = 40;

float targetLeftEyeX = leftEyeX;
float targetRightEyeX = rightEyeX;
const float moveSpeed = 0.1;

int blinkState = 0;
int blinkDelay = 4000;
unsigned long lastBlinkTime = 0;
unsigned long moveTime = 0;

int expression = 1;


void drawExpression(int eyeX, int eyeY, int eyeWidth, int eyeHeight, int exp) {
  display.fillRect(eyeX, eyeY, eyeWidth, eyeHeight);
}


void blink(){
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


void setup() {
  display.init();
  display.flipScreenVertically();
  display.setContrast(255);
  display.clear();
  display.display();
  delay(2000);
}

void loop() {
  blink();
}

