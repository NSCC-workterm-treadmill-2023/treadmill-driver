#include <LiquidCrystal_I2C.h>

#define ENABLE 2
#define RAISE 3
#define LOWER 4
#define BUTTON 5
#define T3 7

enum Direction {UP, DOWN};

int buttonState;
int direction = UP;

int barPins[] = {PD9, PD8, PA2, PB13, PB10, PB15, PB6, PB2 /*PA11, PA12/*PC0, PC1*/};

LiquidCrystal_I2C lcd(0x27, 16, 2);

void lcdPrint(int input) {
  lcd.setCursor(0, 0);
  lcd.print("     ");
  lcd.setCursor(0, 0);
  lcd.print(input);
}

int inclineAsPercentage(int raw) {
  return map(raw, 156, 796, 0, 15);
}

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Moo");

  pinMode(ENABLE, OUTPUT);
  pinMode(RAISE,  OUTPUT);
  pinMode(LOWER,  OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(T3,     OUTPUT);

  digitalWrite(T3, HIGH);
  digitalWrite(ENABLE, HIGH);

  buttonState = digitalRead(BUTTON);
  Serial.println("MOOOO!");
}

void loop() {
  lcdPrint(inclineAsPercentage(analogRead(A1)));
  lcd.print("%");

  int newState1 = digitalRead(BUTTON);
  if (newState1 == buttonState) return; // No change

  delay(10);

  int newState2 = digitalRead(BUTTON);
  if (newState2 != newState1) return; // Just a flutter, not a proper press

  buttonState = newState2;
  Serial.print("  Debounce complete, state is ");
  Serial.println(buttonState);

  if (buttonState) {
    digitalWrite(direction == UP ? RAISE : LOWER, HIGH);
  } else {
    digitalWrite(direction == UP ? RAISE : LOWER, LOW);
    direction = direction == UP ? DOWN : UP;
  }
}