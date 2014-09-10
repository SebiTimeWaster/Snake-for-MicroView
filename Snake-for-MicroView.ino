#include <MicroView.h>

// compiler constants defining game parameters
#define screenSizeX     64
#define screenSizeY     48
#define bufferSize     768 // screenSizeX * screenSizeY / 4 (-2 bit, handled by pointer)
#define tickDelay       50 // game tick = delay in ms / 2
#define pointsPerApple  50 // how much longer the snake will become per apple

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
}

void loop() {
  if(gamestatus == 0) tick();
  if(gamestatus == 1) {
    uView.rectFill(2, 14, 60, 20, BLACK, NORM);
    uView.setFontType(1);
    uView.setCursor(6,18);
    uView.print("LOOSER");
    uView.rect(4, 16, 56, 16);
    uView.display();
    for(int x = 0; x < 10; x++) {
      delay(tickDelay * 4);
      uView.invert(true);
      delay(tickDelay * 4);
      uView.invert(false);
    }
    gamestatus = 0;
    initGame();
  }
  if(gamestatus == 2) {
    // cannot be, cheater!
  }
}

void tick() {
  delay(tickDelay);
  uView.pixel(applePosX, applePosY, BLACK, NORM);
  uView.display();
  delay(tickDelay);
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
  if(!isPlayer) {
    autoPilot();
    // check if user pressed key
  } else {
    // check userinput
  }
  // move and draw head
  moveInDirection(&snakeHeadPosX, &snakeHeadPosY, snakeHeadDir);
  checkCollision();
  if(gamestatus == 0) {
    uView.pixel(snakeHeadPosX, snakeHeadPosY);
    changeValue(&snakeHeadPointer, 1, bufferSize - 1);
    setMovement(snakeHeadPointer, snakeHeadDir);
    if(snakeHeadPosX == applePosX && snakeHeadPosY == applePosY) {
      applePoints += pointsPerApple;
      setNewApple();
    }
  }
  // draw apple
  uView.pixel(applePosX, applePosY);
  uView.display();
}

void initGame() {
  uView.clear(PAGE);
  // set initial position of snake
  snakeHeadPosX = 4;
  snakeHeadPosY = 3;
  snakeHeadPointer = 0;
  snakeTailPosX = 3;
  snakeTailPosY = 3;
  snakeTailPointer = 0;
  snakeMovements[0] = 0b00000000; // 3,3 to 4,3 is always first move
  // set random apple position
  setNewApple();
  applePoints = 0;
  // draw initial game screen
  uView.pixel(3, 3);
  uView.pixel(4, 3);
  uView.pixel(applePosX, applePosY);
  uView.display();
}

void checkCollision() {
  if(snakeHeadPosX > screenSizeX - 1 || snakeHeadPosX < 0 || 
     snakeHeadPosY > screenSizeY - 1 || snakeHeadPosY < 0) gamestatus = 1;
  if((snakeHeadPosX != applePosX || snakeHeadPosY != applePosY) && getPixel(snakeHeadPosX, snakeHeadPosY) == 1) gamestatus = 1;
}

void autoPilot() {
  if(snakeHeadDir == 0) {
         if(applePosX == snakeHeadPosX && applePosY < snakeHeadPosY) snakeHeadDir--;
    else if(applePosX == snakeHeadPosX && applePosY > snakeHeadPosY) snakeHeadDir++;
    else if(applePosX < snakeHeadPosX && applePosY < snakeHeadPosY) snakeHeadDir--;
    else if(applePosX < snakeHeadPosX && applePosY > snakeHeadPosY) snakeHeadDir++;
  } else if(snakeHeadDir == 1) {
         if(applePosY == snakeHeadPosY && applePosX > snakeHeadPosX) snakeHeadDir--;
    else if(applePosY == snakeHeadPosY && applePosX < snakeHeadPosX) snakeHeadDir++;
    else if(applePosY < snakeHeadPosY && applePosX < snakeHeadPosX) snakeHeadDir++;
    else if(applePosY < snakeHeadPosY && applePosX > snakeHeadPosX) snakeHeadDir--;
  } else if(snakeHeadDir == 2) {
         if(applePosX == snakeHeadPosX && applePosY < snakeHeadPosY) snakeHeadDir++;
    else if(applePosX == snakeHeadPosX && applePosY > snakeHeadPosY) snakeHeadDir--;
    else if(applePosX > snakeHeadPosX && applePosY < snakeHeadPosY) snakeHeadDir++;
    else if(applePosX > snakeHeadPosX && applePosY > snakeHeadPosY) snakeHeadDir--;
  } else if(snakeHeadDir == 3) {
         if(applePosY == snakeHeadPosY && applePosX > snakeHeadPosX) snakeHeadDir++;
    else if(applePosY == snakeHeadPosY && applePosX < snakeHeadPosX) snakeHeadDir--;
    else if(applePosY > snakeHeadPosY && applePosX < snakeHeadPosX) snakeHeadDir--;
    else if(applePosY > snakeHeadPosY && applePosX > snakeHeadPosX) snakeHeadDir++;
  }
  if(snakeHeadDir > 3) snakeHeadDir = 0;
  if(snakeHeadDir < 0) snakeHeadDir = 3;
}

void setNewApple() {
  applePosX = random(screenSizeX);
  applePosY = random(screenSizeY);
}

void moveInDirection(int *targetX, int *targetY, int dir) {
  if(dir == 0) *targetX += 1;
  if(dir == 1) *targetY += 1;
  if(dir == 2) *targetX -= 1;
  if(dir == 3) *targetY -= 1;
}

void changeValue(int *target, int value, int maximum) {
  *target += value;
  if(*target > maximum) *target -= maximum;
  if(*target < 0) *target += maximum;
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

