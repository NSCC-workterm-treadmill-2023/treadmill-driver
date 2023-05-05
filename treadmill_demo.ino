#define ENABLE 2
#define RAISE 3
#define LOWER 4
#define BUTTON 5

enum Direction {UP, DOWN};

int buttonState;
int direction = UP;

void setup() {
  Serial.begin(9600);

  pinMode(ENABLE, OUTPUT);
  pinMode(RAISE, OUTPUT);
  pinMode(LOWER, OUTPUT);
  pinMode(BUTTON, INPUT);

  digitalWrite(ENABLE, HIGH);

  buttonState = digitalRead(BUTTON);
}

void loop() {
  int newState1 = digitalRead(BUTTON);
  if (newState1 == buttonState) return; // No change

  delay(10);

  int newState2 = digitalRead(BUTTON);
  if (newState2 != newState1) return; // Just a flutter, not a proper press

  buttonState = newState2;

  if (buttonState) {
    digitalWrite(direction == UP ? RAISE : LOWER, HIGH);
  } else {
    digitalWrite(direction == UP ? RAISE : LOWER, LOW);
    direction = direction == UP ? DOWN : UP;
  }
}