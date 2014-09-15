
/*
 * set this to true to have a bad time (or a good time, depends on your definition) ^^
 */
#define doItHard false


/*
 * Do not change anything below here unless you know what you do
 */
#include <MicroView.h>

// compiler directives defining game parameters
#if doItHard
  #define screenSizeX        64
  #define screenSizeY        48
  #define bufferSize        713 // (screenSizeX - 2) * (screenSizeY - 2) / 4
  #define maxBufferLength  2850 // (screenSizeX - 2) * (screenSizeY - 2) - 2
  #define pointsPerApple      5 // how much longer the snake will become per apple
  #define doublePixels    false
#else
  #define screenSizeX        32
  #define screenSizeY        24
  #define bufferSize        165 // (screenSizeX - 2) * (screenSizeY - 2) / 4
  #define maxBufferLength   658 // (screenSizeX - 2) * (screenSizeY - 2) - 2
  #define pointsPerApple      3 // how much longer the snake will become per apple
  #define doublePixels     true
#endif
#define tickDelay           125 // game tick in ms = 8 fps (more or less)
#define userDelay          1000 // how long to screens before / after user interaction

// global vars
uint8_t      *screenBuffer;
int           snakeHeadPosX              = 0;
int           snakeHeadPosY              = 0;
int           snakeHeadDir               = 0;
int           snakeHeadPointer           = 0;
int           snakeTailPosX              = 0;
int           snakeTailPosY              = 0;
int           snakeTailDir               = 0;
int           snakeTailPointer           = 0;
int           applePosX                  = 0;
int           applePosY                  = 0;
int           applePoints                = 0;
byte          snakeMovements[bufferSize] = {0};
byte          gamestatus                 = 0; // 0=running, 1=lost (collision), 2=won (very unlikely)
bool          isPlayer                   = false;
unsigned long startTime                  = 0;
long          timeLeftInTick             = 0;
uint8_t       appleColor                 = BLACK;
unsigned long buttonTime                 = 0;
volatile byte buttonDir                  = 0;
byte          aniCounter                 = 0;

void setup() {
  // init random generator
  randomSeed(analogRead(0));
  // init microview library
  uView.begin();
  uView.clear(ALL);
  uView.clear(PAGE);
  // point to uView libraries internal screen buffer
  screenBuffer = uView.getScreenBuffer();
  // configure button input pins
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  // attach interrupt for button presses
  attachInterrupt(0, buttonRight, FALLING);
  attachInterrupt(1, buttonLeft, FALLING);
  // init game
  initGame();
}

void loop() {
  // decide what to do (what status the game is in)
  // game loop
  if(gamestatus == 0) tick();
  // user lost
  if(gamestatus == 1) {
    // show looser screen
    aniCounter++;
    showLooserAnimation();
    // user wants to play
    if(buttonDir > 0) isPlayer = true;
    else if(isPlayer) {
      // when animation showed 10 times user does not want to play anymore
      if(aniCounter == 10) isPlayer = false;
      // if player show animation again
      else return;
    }
    // restart game
    aniCounter = 0;
    gamestatus = 0;
    initGame();
  }
  // user won
  if(gamestatus == 2) {
    // cannot be, cheater!
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
  pixel(4, 4);
  pixel(5, 4);
  pixel(applePosX, applePosY);
  rect(0, 0, screenSizeX, screenSizeY);
  uView.display();
  if(isPlayer) {
    // give time without movement so user can adjust to new position
    for(byte x = 0; x < userDelay / tickDelay; x++) {
      appleColor = !appleColor;
      pixel(applePosX, applePosY, appleColor);
      uView.display();
      delay(tickDelay);
    }
  }
  // reset button press
  buttonDir = 0;
  // set first tick time measurement
  startTime = millis();
}

void tick() {
  // get last executed movements
  snakeHeadDir = getMovement(snakeHeadPointer);
  snakeTailDir = getMovement(snakeTailPointer);
  // move and draw tail
  if(applePoints == 0) {
    pixel(snakeTailPosX, snakeTailPosY, BLACK);
    moveInDirection(&snakeTailPosX, &snakeTailPosY, snakeTailDir);
    changeValue(&snakeTailPointer, 1, bufferSize - 1);
  } else applePoints--;
  // move snake
  if(!isPlayer) autoPilot();
  else if(buttonDir > 0) {
    if(buttonDir == 2) changeValue(&snakeHeadDir, 1, 3);
    if(buttonDir == 1) changeValue(&snakeHeadDir, -1, 3);
    buttonDir = 0;
  }
  // move and draw head and check for collisions
  moveInDirection(&snakeHeadPosX, &snakeHeadPosY, snakeHeadDir);
  checkCollision();
  if(gamestatus == 0) {
    pixel(snakeHeadPosX, snakeHeadPosY);
    changeValue(&snakeHeadPointer, 1, bufferSize - 1);
    setMovement(snakeHeadPointer, snakeHeadDir);
  } else {
    pixel(snakeHeadPosX, snakeHeadPosY, BLACK);
  }
  // draw apple
  appleColor = !appleColor;
  pixel(applePosX, applePosY, appleColor);
  uView.display();
  // slack of the rest of the needed tick time
  timeLeftInTick = tickDelay - (millis() - startTime);
  delay(max(timeLeftInTick, 0));
  startTime = millis();
  // check if player wants to join game
  if(!isPlayer && buttonDir > 0) {
    buttonDir = 0;
    isPlayer = true;
    initGame();
  }
}

void showLooserAnimation() {
  // when user presses some button, stop animation. max delay is ~120 ms, which feels instant.
  for(byte x = 0; x < 5; x++) {
    delay(userDelay / 10);
    if(aniCounter > 1 && buttonDir > 0) return;
  }
  uView.rectFill(4, 16, 56, 16, BLACK, NORM);
  uView.setFontType(1);
  uView.setCursor(6,18);
  uView.print("LOOSER");
  uView.display();
  for(byte x = 0; x < 16; x++) {
    delay(10);
    uView.rect(max(3 - x, 0), 15 - x, min(58 + x * 2, 64), 18 + x * 2);
    uView.display();
  }
  if(aniCounter == 1) buttonDir = 0;
  if(buttonDir > 0) return;
  delay(100);
  if(buttonDir > 0) return;
  for(byte x = 0; x < 16; x++) {
    delay(10);
    uView.rect(max(3 - x, 0), 15 - x, min(58 + x * 2, 64), 18 + x * 2, BLACK, NORM);
    uView.display();
  }
  for(byte x = 0; x < 5; x++) {
    delay(userDelay / 10);
    if(buttonDir > 0) return;
  }
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

void rect(byte x, byte y, byte width, byte height) {
  rect(x, y, width, height, WHITE);
}

void rect(byte x, byte y, byte width, byte height, uint8_t color) {
  if(doublePixels) {
    uView.rect(x * 2, y * 2, width * 2, height * 2, color, NORM);
    uView.rect(x * 2 + 1, y * 2 + 1, width * 2 - 2, height * 2 - 2, color, NORM);
  } else {
    uView.rect(x, y, width, height, color, NORM);
  }
}

void pixel(byte x, byte y) {
  pixel(x, y, WHITE);
}

void pixel(byte x, byte y, uint8_t color) {
  if(doublePixels) uView.rect(x * 2, y * 2, 2, 2, color, NORM);
  else uView.pixel(x, y, color, NORM);
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

void buttonLeft() {
  if(millis() - buttonTime > 50) { // lock keys for 50 ms
    buttonTime = millis();
    buttonDir = 1;
  }
}

void buttonRight() {
  if(millis() - buttonTime > 50) { // lock keys for 50 ms
    buttonTime = millis();
    buttonDir = 2;
  }
}

byte getMovement(int pos) {
  return snakeMovements[pos / 4] >> pos % 4 * 2 & 3;
}

void setMovement(int pos, byte movement) {
  snakeMovements[pos / 4] = (snakeMovements[pos / 4] & (~(3 << (pos % 4 * 2)))) | (movement << (pos % 4 * 2));
}

byte getPixel(int x, int y) {
  if(doublePixels) {
    x *= 2;
    y *= 2;
  }
  return (byte)screenBuffer[x + y / 8 * 64] >> (y - y / 8 * 8) & 0x01;
}

