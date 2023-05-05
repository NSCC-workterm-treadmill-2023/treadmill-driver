#define ENABLE 2
#define RAISE 3
#define LOWER 4
#define BUTTON 5

int state;

void setup() {
  Serial.begin(9600);

  pinMode(ENABLE, OUTPUT);
  pinMode(RAISE, OUTPUT);
  pinMode(LOWER, OUTPUT);
  pinMode(BUTTON, INPUT);

  // digitalWrite(ENABLE, HIGH);

  // digitalWrite(RAISE, HIGH);
  // delay(3000);
  // digitalWrite(RAISE, LOW);
  // delay(1000);
  // digitalWrite(LOWER, HIGH);
  // delay(3000);
  // digitalWrite(LOWER, LOW);

  // digitalWrite(ENABLE, LOW);

  state = digitalRead(BUTTON);
}

void loop() {
  int newState1 = digitalRead(BUTTON);
  if (newState1 == state) return; // No change

  delay(10);

  int newState2 = digitalRead(BUTTON);
  if (newState2 != newState1) return; // Just a flutter, not a proper press

  state = newState2;
  Serial.print("Button ");
  Serial.println(state ? "on" : "off");
}