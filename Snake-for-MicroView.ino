#include <MicroView.h>

// compiler directives defining game parameters
// HARD GAME
#define hardGameScreenSizeX       64 // screen width in pixels
#define hardGameScreenSizeY       48 // screen height in pixels
#define hardGameBufferSize       713 // (screenSizeX - 2) * (screenSizeY - 2) / 4
#define hardGamePointsPerApple     5 // how much longer the snake will become per apple
#define hardGameDoublePixels   false // if pixels should be shown double in size; this somewhat counteracts the purpose of having an arbitrary screen size,
                                     // but implementing an interpolation algorythm would be slow and would produce bad visual results
#define hardGameSpeedIncrease      1 // how much the speed increases per eaten apple in ms
#define hardGameMinTickDelay      75 // the minimum tick delay to cap speed increase, the ATmega328P is capable of such high speeds that it is unplayable
// EASY GAME
#define easyGameScreenSizeX       32
#define easyGameScreenSizeY       24
#define easyGameBufferSize       165
#define easyGamePointsPerApple     3
#define easyGameDoublePixels    true
#define easyGameSpeedIncrease      0
#define easyGameMinTickDelay      75
// the rest
#define gameTickDelay            125 // game tick in ms, 125 ms = 8 fps (more or less)
#define userDelay               2000 // general delay for animations and user interaction
#define keyLock                   25 // how long to lock the keys after key press to avoid bouncing


/**
 *  --------------------------------------------------------------
 *  Do not change anything below here unless you know what you do!
 *  --------------------------------------------------------------
 */


// global vars
uint8_t      *screenBuffer;
byte          tickDelay                          = 0;
byte          minTickDelay                       = 0;
byte          screenSizeX                        = 0;
byte          screenSizeY                        = 0;
int           bufferSize                         = 0;
byte          pointsPerApple                     = 0;
bool          doublePixels                       = false;
byte          speedIncrease                      = 0;
int           snakeHeadPosX                      = 0;
int           snakeHeadPosY                      = 0;
int           snakeHeadDir                       = 0;
int           snakeHeadPointer                   = 0;
int           snakeTailPosX                      = 0;
int           snakeTailPosY                      = 0;
int           snakeTailDir                       = 0;
int           snakeTailPointer                   = 0;
int           applePosX                          = 0;
int           applePosY                          = 0;
int           applePoints                        = 0;
int           applesCollected                    = 0;
byte          snakeMovements[hardGameBufferSize] = {0};
bool          gameIsRunning                      = true;
bool          isPlayer                           = false;
unsigned long startTime                          = 0;
long          timeLeftInTick                     = 0;
uint8_t       appleColor                         = BLACK;
unsigned long buttonTime                         = 0;
volatile byte buttonDir                          = 0;
byte          aniCounter                         = 0;

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
  // set game parameters
  setGameMode(true);
  // init game
  initGame();
}

void loop() {
  // decide what to do (what status the game is in)
  // game loop
  if(gameIsRunning) tick();
  // user lost
  else {
    // show looser screen
    aniCounter++;
    showLoserAnimation();
    // user wants to play
    if(buttonDir > 0) {
      isPlayer = true;
      // set game parameters
      if(buttonDir == 1) setGameMode(true);
      if(buttonDir == 2) setGameMode(false);
    } else if(isPlayer) {
      // when animation showed 10 times user does not want to play anymore
      if(aniCounter == 10) {
        isPlayer = false;
        // set game parameters
        setGameMode(true);
      }
      // if player show animation again
      else return;
    }
    // restart game
    aniCounter = 0;
    gameIsRunning = true;
    initGame();
  }
}

void initGame() {
  tickDelay = gameTickDelay;
  // clear screen
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
  applesCollected = 0;
  // draw initial game screen
  pixel(4, 4);
  pixel(5, 4);
  pixel(applePosX, applePosY);
  rect(0, 0, screenSizeX, screenSizeY);
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
  // move and draw tail if no applepoints left
  if(applePoints == 0) {
    pixel(snakeTailPosX, snakeTailPosY, BLACK);
    moveInDirection(&snakeTailPosX, &snakeTailPosY, snakeTailDir);
    changeValue(&snakeTailPointer, 1, bufferSize - 1);
  } else applePoints--;
  // set new snake head direction
  if(!isPlayer) autoPilot();
  else if(buttonDir > 0) {
    if(buttonDir == 2) changeValue(&snakeHeadDir, 1, 3);
    if(buttonDir == 1) changeValue(&snakeHeadDir, -1, 3);
    buttonDir = 0;
  }
  // move and draw head and check for collisions
  moveInDirection(&snakeHeadPosX, &snakeHeadPosY, snakeHeadDir);
  checkCollision();
  if(gameIsRunning) {
    pixel(snakeHeadPosX, snakeHeadPosY);
    changeValue(&snakeHeadPointer, 1, bufferSize - 1);
    setMovement(snakeHeadPointer, snakeHeadDir);
  } else {
    // if collision paint head black to visualize crash
    pixel(snakeHeadPosX, snakeHeadPosY, BLACK);
  }
  // draw apple (blinks with 1 hertz = tickDelay * 2)
  appleColor = !appleColor;
  pixel(applePosX, applePosY, appleColor);
  // draw screen
  uView.display();
  // slack of the rest of the needed tick time to assure correct fps
  timeLeftInTick = tickDelay - (millis() - startTime);
  delay(max(timeLeftInTick, 0));
  startTime = millis();
  // check if player wants to join game
  if(!isPlayer && buttonDir > 0) {
    // set game parameters
    if(buttonDir == 1) setGameMode(true);
    if(buttonDir == 2) setGameMode(false);
    buttonDir = 0;
    isPlayer = true;
    initGame();
  }
}

void setGameMode(bool hardGame) {
  // set game parameters according to users wish (i.e. hard or easy game)
  if(hardGame) {
    screenSizeX    = hardGameScreenSizeX;
    screenSizeY    = hardGameScreenSizeY;
    bufferSize     = hardGameBufferSize;
    pointsPerApple = hardGamePointsPerApple;
    doublePixels   = hardGameDoublePixels;
    speedIncrease  = hardGameSpeedIncrease;
    minTickDelay   = hardGameMinTickDelay;
  } else {
    screenSizeX    = easyGameScreenSizeX;
    screenSizeY    = easyGameScreenSizeY;
    bufferSize     = easyGameBufferSize;
    pointsPerApple = easyGamePointsPerApple;
    doublePixels   = easyGameDoublePixels;
    speedIncrease  = easyGameSpeedIncrease;
    minTickDelay   = easyGameMinTickDelay;
  }
}

void showLoserAnimation() {
  // when user presses some button, stop animation no matter where. max delay is ~120 ms, which feels instant.
  // wait half an userDelay (in 10th of a userDelay steps)
  for(byte x = 0; x < 5; x++) {
    delay(userDelay / 10);
    // do not break delay on first round, so user can see points before proceeding
    if(aniCounter > 1 && buttonDir > 0) return;
  }
  // draw points
  uView.rectFill(0, 16, 64, 16, BLACK, NORM);
  uView.setFontType(1);
  uView.setCursor(2, 18);
  if(applesCollected < 100) uView.print("0");
  if(applesCollected < 10) uView.print("0");
  uView.print(applesCollected);
  uView.print(" Pts");
  uView.display();
  // show screen fill animation (~ 192 ms)
  for(byte x = 0; x < 16; x++) {
    delay(10);
    uView.rect(0, 15 - x, 64, 18 + x * 2);
    uView.display();
  }
  // on first round delete user input assuming he pressed by accident while playing
  if(aniCounter == 1) buttonDir = 0;
  if(buttonDir > 0) return;
  // wait a bit in between anímation
  delay(100);
  if(buttonDir > 0) return;
  // show screen fill animation (~ 192 ms)
  for(byte x = 0; x < 16; x++) {
    delay(10);
    uView.rect(0, 15 - x, 64, 18 + x * 2, BLACK, NORM);
    uView.display();
  }
  // wait half an userDelay (in 10th of a userDelay steps)
  for(byte x = 0; x < 5; x++) {
    delay(userDelay / 10);
    if(buttonDir > 0) return;
  }
}

void checkCollision() {
  // if applePos and snakeHeadPos are the same user ate apple
  if(snakeHeadPosX == applePosX && snakeHeadPosY == applePosY) {
    applePoints += pointsPerApple;
    setNewApple();
    applesCollected++;
    if(tickDelay >= speedIncrease && tickDelay > minTickDelay) tickDelay -= speedIncrease;
    return;
  }
  // check if there is a bright pixel on snakeHeadPos (collision)
  if(getPixel(snakeHeadPosX, snakeHeadPosY)) gameIsRunning = false;
}

void autoPilot() {
  // a simple state machine, goes greedy after the apple and will collide with itself
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
  // find an empty spot for the apple
  // TODO: should be optimized! at the moment this will be incredibly slow in a late state of the game
  do {
    applePosX = random(1, screenSizeX - 1);
    applePosY = random(1, screenSizeY - 1);
  } while(getPixel(applePosX, applePosY));
}

void rect(byte x, byte y, byte width, byte height) {
  // call overload
  rect(x, y, width, height, WHITE);
}

void rect(byte x, byte y, byte width, byte height, uint8_t color) {
  // draw a rect, if doublePixels draw two
  if(doublePixels) {
    uView.rect(x * 2, y * 2, width * 2, height * 2, color, NORM);
    uView.rect(x * 2 + 1, y * 2 + 1, width * 2 - 2, height * 2 - 2, color, NORM);
  } else {
    uView.rect(x, y, width, height, color, NORM);
  }
}

void pixel(byte x, byte y) {
  // call overload
  pixel(x, y, WHITE);
}

void pixel(byte x, byte y, uint8_t color) {
  // draw a pixel, if doublePixels draw a two by two pixel
  if(doublePixels) uView.rect(x * 2, y * 2, 2, 2, color, NORM);
  else uView.pixel(x, y, color, NORM);
}

void moveInDirection(int *targetX, int *targetY, int dir) {
  // move snake dependent of direction given
  if(dir == 0) *targetX += 1;
  if(dir == 1) *targetY += 1;
  if(dir == 2) *targetX -= 1;
  if(dir == 3) *targetY -= 1;
}

void changeValue(int *target, int value, int maximum) {
  // changes target by value while rolling over when reaching maximum
  *target += value;
  if(*target > maximum) *target -= (maximum + 1);
  if(*target < 0) *target += (maximum + 1);
}

void buttonLeft() {
  // left button was pressed, lock keys for keyLock ms
  if(millis() - buttonTime > keyLock) {
    buttonTime = millis();
    buttonDir = 1;
  }
}

void buttonRight() {
  // right button was pressed, lock keys for keyLock ms
  if(millis() - buttonTime > keyLock) {
    buttonTime = millis();
    buttonDir = 2;
  }
}

byte getMovement(int pos) {
  // read one movement out of the movement buffer
  return snakeMovements[pos / 4] >> pos % 4 * 2 & 3;
}

void setMovement(int pos, byte movement) {
  // write one movement into the movement buffer
  snakeMovements[pos / 4] = (snakeMovements[pos / 4] & (~(3 << (pos % 4 * 2)))) | (movement << (pos % 4 * 2));
}

byte getPixel(int x, int y) {
  // get pixel value out of uView library's internal screen buffer
  if(doublePixels) {
    x *= 2;
    y *= 2;
  }
  return screenBuffer[x + y / 8 * 64] >> (y - y / 8 * 8) & 0x01;
}

