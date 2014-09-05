#define CELL(row, col) (row)*numCols + (col)

const byte rowPins[] = { 2, 3, 4, 5, 17, 16, 15, 14 };
const byte colPins[] = { 6, 7, 8, 9, 10, 11, 12, 13 };
const byte potPin = 5;
const byte numRows = sizeof(rowPins) / sizeof(*rowPins);
const byte numCols = sizeof(colPins) / sizeof(*colPins);
const int bufferSize = numCols*numRows;

const int delta[4] = { 1, numCols, -1, -numCols };
const int maxDelta = sizeof(delta) / sizeof(*delta) - 1;

const int tick = 75;
int ticks;

const int leftThreshold = 600;
const int rightThreshold = 900;

bool screen[bufferSize];

int snake[bufferSize];
int *pSnakeHead, *pSnakeTail;
int curDelta;

int apple;

long nextTick;
int playing;

void setup() {
  byte row, col;

  for (row = 0; row < numRows; row++) {
    pinMode(rowPins[row], OUTPUT);
  }

  for (col = 0; col < numCols; col++) {
    pinMode(colPins[col], OUTPUT);
    digitalWrite(colPins[col], HIGH);
  }

  Serial.begin(9600);

  initScreen();
  initSnake();

  initRandom();
  initApple();

  ticks = 0;
  nextTick = tick;
  playing = true;
}

void initScreen() {
  byte i;
  for (i = 0; i < bufferSize; i++) {
    screen[i] = LOW;
  }
}

void initApple() {
  int i;
  for (i = 0; i < 10; i++) {
    apple = random(bufferSize);
    if (screen[apple] == HIGH) continue;

    screen[apple] = HIGH;
    break;
  }

  // TODO: restart?
}

void initSnake() {
  snake[0] = CELL(3, 3);
  snake[1] = CELL(3, 4);
  pSnakeTail = snake;
  pSnakeHead = snake + 1;
  curDelta = 0;

  screen[*pSnakeHead] = HIGH;
  screen[*pSnakeTail] = HIGH;
}

void initRandom() {
  int seed = analogRead(potPin);
  randomSeed(seed);
  Serial.print("seed: ");
  Serial.println(seed);
}

void loop() {
  nextTurn();
  refreshScreen();
}

void nextTurn() {
  long now = millis();
  if (now > nextTick) {
    if (playing) {
      screen[*pSnakeHead] = ticks % 2 == 0 ? HIGH : LOW;
      screen[apple] = ticks % 5 > 0 ? HIGH : LOW;
      if (ticks % 10 == 0) {
        int input = analogRead(potPin);
        if (input < leftThreshold) {
          turnLeft();
        }
        else if (input > rightThreshold) {
          turnRight();
        }

        if (collides()) {
          playing = false;
        }
        else {
          crawl();
        }
      }
    }
    else {
      if (ticks % 10 == 0) {
        initScreen();
        initSnake();
        initApple();
        playing = true;
      }
      else {
        int *p = pSnakeTail;
        while (true) {
          screen[*p] = ticks % 2 == 0 ? HIGH : LOW;
          if (p == pSnakeHead) break;

          p++;
          if (p > snake + bufferSize) {
            p = snake;
          }
        }
      }
    }

    nextTick += tick;
    ticks++;

    Serial.println(analogRead(potPin));
  }
}

bool collides() {
  int newHeadPos = *pSnakeHead + delta[curDelta];
  return (newHeadPos < 0 || newHeadPos >= bufferSize
         || (newHeadPos % numCols) != (*pSnakeHead % numCols)
             && (newHeadPos / numCols) != (*pSnakeHead / numCols)
         || screen[newHeadPos] == HIGH && newHeadPos != apple);
}

void turnRight() {
  curDelta++;
  if (curDelta > maxDelta) {
    curDelta = 0;
  }
}

void turnLeft() {
  curDelta--;
  if (curDelta < 0) {
    curDelta = maxDelta;
  }
}

void crawl() {
  int pos;
  pos = *pSnakeHead;

  pSnakeHead++;
  if (pSnakeHead >= snake + bufferSize) {
    pSnakeHead = snake;
  }

  *pSnakeHead = pos + delta[curDelta];

  screen[*pSnakeHead] = HIGH;

  if (*pSnakeHead == apple) {
    initApple();
  }
  else {
    screen[*pSnakeTail] = LOW;

    pSnakeTail++;
    if (pSnakeTail >= snake + bufferSize) {
      pSnakeTail = snake;
    }
  }
}

void refreshScreen() {
  byte row, col;
  for (row = 0; row < numRows; row++) {
    digitalWrite(rowPins[row], HIGH);
    for (col = 0; col < numCols; col++) {
      digitalWrite(colPins[col], !screen[CELL(row, col)]);
      if (screen[CELL(row, col)] == HIGH) {
        digitalWrite(colPins[col], HIGH);
      }
    }
    digitalWrite(rowPins[row], LOW);
  }
}
