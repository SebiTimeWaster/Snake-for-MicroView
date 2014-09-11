#include <MicroView.h>

// compiler constants defining game parameters
#define screenSizeX      64
#define screenSizeY      48
#define bufferSize      713 // (screenSizeX - 2) * (screenSizeY - 2) / 4 (-2 bit, handled by pointer)
#define tickDelay       125 // game tick in ms = 8 fps (more or less)
#define pointsPerApple   5 // how much longer the snake will become per apple
#define looserDelay    2000 // how long to show looser screen in ms
#define startDelay     1000 // how long to show start screen to user in ms

// global vars holding game status
int  snakeHeadPosX;
int  snakeHeadPosY;
int  snakeHeadDir;
int  snakeHeadPointer;
int  snakeTailPosX;
int  snakeTailPosY;
int  snakeTailDir;
int  snakeTailPointer;
int  applePosX;
int  applePosY;
int  applePoints;
byte snakeMovements[bufferSize] = {0};
byte gamestatus = 0; // 0=running, 1=lost (collision), 2=won (very unlikely)
bool isPlayer = false;
unsigned long startTime = 0;
long timeLeftInTick = 0;
uint8_t appleColor = BLACK;

// screen buffer pointer
uint8_t *screenBuffer;

void setup() {
  // init random generator
  randomSeed(analogRead(0));
  // init microview library
  uView.begin();
  uView.clear(ALL);
  uView.clear(PAGE);
  screenBuffer = uView.getScreenBuffer();
  // init game
  initGame();
  Serial.begin(9600);
}

void loop() {
  if(gamestatus == 0) tick();
  if(gamestatus == 1) {
    // show looser screen
    uView.rectFill(4, 16, 56, 16, BLACK, NORM);
    uView.setFontType(1);
    uView.setCursor(6,18);
    uView.print("LOOSER");
    uView.display();
    for(byte x = 0; x < 15; x++) {
      delay(20);
      uView.rect(max(3 - x, 0), 15 - x, min(58 + x * 2, 64), 18 + x * 2);
      uView.display();
    }
    delay(looserDelay);
    if(isPlayer) {
      uView.clear(PAGE);
      // check if user pressed key else 
      return;
    }
    // restart game
    gamestatus = 0;
    initGame();
  }
  if(gamestatus == 2) {
    // cannot be, cheater!
  }
}

void tick() {
  // get last executed movements
  snakeHeadDir = getMovement(snakeHeadPointer);
  snakeTailDir = getMovement(snakeTailPointer);
  // move and draw tail
  if(applePoints == 0) {
    uView.pixel(snakeTailPosX, snakeTailPosY, BLACK, NORM);
    moveInDirection(&snakeTailPosX, &snakeTailPosY, snakeTailDir);
    changeValue(&snakeTailPointer, 1, bufferSize - 1);
  } else applePoints--;
  // move snake
  if(!isPlayer) autoPilot();
  else {
    // check userinput
  }
  // move and draw head and check for collisions
  moveInDirection(&snakeHeadPosX, &snakeHeadPosY, snakeHeadDir);
  checkCollision();
  if(gamestatus == 0) {
    uView.pixel(snakeHeadPosX, snakeHeadPosY);
    changeValue(&snakeHeadPointer, 1, bufferSize - 1);
    setMovement(snakeHeadPointer, snakeHeadDir);
  }
  // draw apple
  appleColor = !appleColor;
  uView.pixel(applePosX, applePosY, appleColor, NORM);
  uView.display();
  // slack of the rest of the needed tick time
  timeLeftInTick = tickDelay - (millis() - startTime);
  delay(max(timeLeftInTick, 0));
  startTime = millis();
  // check if player wants to join game
  if(!isPlayer) {
    // check if user pressed key
    // isPlayer = true;
    // initGame();
  }
}

void initGame() {
  uView.clear(PAGE);
  // set initial position of snake
  snakeHeadPosX = 5;
  snakeHeadPosY = 4;
  snakeHeadPointer = 0;
  snakeTailPosX = 4;
  snakeTailPosY = 4;
  snakeTailPointer = 0;
  snakeMovements[0] = 0b00000000; // 4,4 to 5,4 is always first move
  // set random apple position
  setNewApple();
  applePoints = 0;
  // draw initial game screen
  uView.pixel(4, 4);
  uView.pixel(5, 4);
  uView.pixel(applePosX, applePosY);
  uView.rect(0, 0, 64, 48);
  uView.display();
  if(isPlayer) {
    // give time without movement so user can adjust to new pos
    for(byte x = 0; x < startDelay / tickDelay; x++) {
      appleColor = !appleColor;
      uView.pixel(applePosX, applePosY, appleColor, NORM);
      uView.display();
      delay(tickDelay);
    }
  }
  // set first tick time measurement
  startTime = millis();
}

void checkCollision() {
  if(snakeHeadPosX == applePosX && snakeHeadPosY == applePosY) {
    applePoints += pointsPerApple;
    setNewApple();
    return;
  }
  if(getPixel(snakeHeadPosX, snakeHeadPosY)) gamestatus = 1;
}

void autoPilot() {
  if(snakeHeadDir == 0 && applePosX <= snakeHeadPosX) {
    if(applePosY >= snakeHeadPosY) changeValue(&snakeHeadDir, 1, 3);
    else changeValue(&snakeHeadDir, -1, 3);
  } else if(snakeHeadDir == 1 && applePosY <= snakeHeadPosY) {
    if(applePosX <= snakeHeadPosX) changeValue(&snakeHeadDir, 1, 3);
    else changeValue(&snakeHeadDir, -1, 3);
  } else if(snakeHeadDir == 2 && applePosX >= snakeHeadPosX) {
    if(applePosY <= snakeHeadPosY) changeValue(&snakeHeadDir, 1, 3);
    else changeValue(&snakeHeadDir, -1, 3);
  } else if(snakeHeadDir == 3 && applePosY >= snakeHeadPosY) {
    if(applePosX >= snakeHeadPosX) changeValue(&snakeHeadDir, 1, 3);
    else changeValue(&snakeHeadDir, -1, 3);
  }
}

void setNewApple() {
  // TODO: should be optimized!
  do {
    applePosX = random(1, screenSizeX - 1);
    applePosY = random(1, screenSizeY - 1);
  } while(getPixel(applePosX, applePosY));
}

void moveInDirection(int *targetX, int *targetY, int dir) {
  if(dir == 0) *targetX += 1;
  if(dir == 1) *targetY += 1;
  if(dir == 2) *targetX -= 1;
  if(dir == 3) *targetY -= 1;
}

void changeValue(int *target, int value, int maximum) {
  *target += value;
  if(*target > maximum) *target -= (maximum + 1);
  if(*target < 0) *target += (maximum + 1);
}

byte getMovement(int pos) {
  return snakeMovements[pos / 4] >> pos % 4 * 2 & 3;
}

void setMovement(int pos, byte movement) {
  snakeMovements[pos / 4] = (snakeMovements[pos / 4] & (~(3 << (pos % 4 * 2)))) | (movement << (pos % 4 * 2));
}

byte getPixel(int x, int y) {
  return (byte)screenBuffer[x + y / 8 * 64] >> (y - y / 8 * 8) & 0x01;
}

