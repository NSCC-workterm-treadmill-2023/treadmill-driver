#define ENABLE 2
#define RAISE 3
#define LOWER 4

void setup() {
  pinMode(ENABLE, OUTPUT);
  pinMode(RAISE, OUTPUT);
  pinMode(LOWER, OUTPUT);

  digitalWrite(ENABLE, HIGH);

  digitalWrite(RAISE, HIGH);
  delay(3000);
  digitalWrite(RAISE, LOW);
  delay(1000);
  digitalWrite(LOWER, HIGH);
  delay(3000);
  digitalWrite(LOWER, LOW);

  digitalWrite(ENABLE, LOW);
}

void loop() {
}