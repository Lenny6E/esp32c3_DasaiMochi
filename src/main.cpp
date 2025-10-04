#include <Wire.h>
#include "SH1106Wire.h"

#define OLED_ADDR 0x3C
SH1106Wire display(OLED_ADDR, 8, 9);  // SDA = 8, SCL = 9
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

const int ROWS = 8;
const int COLS = 16;
uint8_t field[ROWS][COLS]; // 8x16 field
int blockWidth  = SCREEN_WIDTH / COLS;   // 8 px
int blockHeight = SCREEN_HEIGHT / ROWS;  // 8 px

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


//------------------------------------------------------
// Eye Functions
//------------------------------------------------------

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

//------------------------------------------------------
// Random Function
//------------------------------------------------------

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

//------------------------------------------------------
// Hacking Rain Function
//------------------------------------------------------
void runHackingRain(unsigned long startTime) {
  // Spaltenbreite in Pixel
  const int colWidth = 8;  
  const int cols = SCREEN_WIDTH / colWidth;  
  int yPositions[cols];  

  // Anfangsposition jeder Spalte
  for (int i = 0; i < cols; i++) {
    yPositions[i] = random(-SCREEN_HEIGHT, 0); 
  }

  while (millis() - startTime < 10000) { // 10 Sekunden "Hack Effekt"
    display.clear();

    for (int i = 0; i < cols; i++) {
      int y = yPositions[i];
      int x = i * colWidth;

      // zeichne eine 0 oder 1 an Position (x,y)
      char c = random(2) ? '0' : '1';
      display.drawString(x, y, String(c));

      // nach unten bewegen
      yPositions[i] += 8;  

      // wenn unten angekommen -> neu starten
      if (yPositions[i] > SCREEN_HEIGHT) {
        yPositions[i] = random(-20, 0);
      }
    }

    display.display();
    delay(80);
  }

  display.clear();
  display.display();
}


//------------------------------------------------------
unsigned long lastAction = 0;
const unsigned long interval = 120000; // every two minutes runs a special function
bool runningSpecial = false;

void setup() {
  display.init();
  display.flipScreenVertically();
  display.setContrast(255);
  display.clear();
  display.display();
  delay(2000);

  randomSeed(analogRead(A0));
}

void loop() {
  unsigned long currentMillis = millis();

  if (!runningSpecial && (currentMillis - lastAction >= interval)) {
    lastAction = currentMillis;
    runningSpecial = true;

    int choice = random(6);
    // 0 = Tetris, 1 = Blocks, 2 = Hacking Rain, 3,4,5 = nothing => 50% chance nothing happens

    if (choice == 0) {
      runRandomTetris();
    } else if (choice == 1) {
      runRandomBlocks(currentMillis);
    } else if (choice == 2) {
      runHackingRain(currentMillis);
    } 

    runningSpecial = false;
  }

  if (!runningSpecial) {
    blink();
  }
}

