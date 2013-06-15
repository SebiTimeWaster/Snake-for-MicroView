const int buttonPin = 4;
const int ledPin = 13;
const int debounceDelay = 50;

int previousButtonState = LOW;
int currentButtonLevel = LOW;
int ledState = LOW;

long lastDebounceTime = 0;

void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  if (debounce(buttonPin, &currentButtonLevel, &lastDebounceTime)) {
    if (currentButtonLevel == HIGH && previousButtonState == LOW) {
      ledState = !ledState;
    }
    previousButtonState = currentButtonLevel;
  }
  digitalWrite(ledPin, ledState);
}

bool debounce(int pin, int *pCurrentLevel, long *pLastDebounceTime) {
  long now = millis();
  int currentLevel = digitalRead(pin);

  if (currentLevel != *pCurrentLevel) {
    *pCurrentLevel = currentLevel;
    *pLastDebounceTime = now;
  }

  return (now - *pLastDebounceTime > debounceDelay);
}
