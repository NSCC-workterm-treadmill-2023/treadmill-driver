#include <LiquidCrystal_I2C.h>
#include <Ethernet.h>
#include <MQTTClient.h>

#define MQTT_CLIENT_NAME "treadmill_controller"
#define MQTT_INTERVAL 100
// A unique ID to use in our MQTT topics, in case we want to connect multiple
// treamills to the same broker.
#define TREADMILL_ID "T9800-1"

#define ENABLE_ELEV_CHANGE 2
#define RAISE 3
#define LOWER 4
#define BUTTON PC13
#define ENABLE_ELEV_READ 7

enum Direction {UP, DOWN};

int buttonState;
int direction = UP;

LiquidCrystal_I2C lcd(0x27, 16, 2);

EthernetClient ethClient;
MQTTClient mqtt = MQTTClient(256); // Param is max packet size
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress localIP(192, 168, 5, 2);
IPAddress brokerIP(192, 168, 5, 1);
uint32_t lastMqttSendTime = 0;

void connectToMQTT() {
  mqtt.begin(brokerIP, 1883, ethClient);

  while (!mqtt.connect(MQTT_CLIENT_NAME)) {
    delay(100);
  }

  if (!mqtt.connected()) {
    return;
  }

  //mqtt.onMessage(messageHandler);
  //mqtt.subscribe(topic);
}

void publish(const char *topicSuffix, const char *message) {
  String topic = String("/");
  topic.concat(TREADMILL_ID);
  topic.concat(topicSuffix);

  mqtt.publish(topic, message);
}

void publish(const char *topicSuffix, long message) {
  publish(topicSuffix, String(message).c_str());
}

long inclineAsPercentage(int ADCReading) {
  // The user manual says that incline is between 0% and 15%
  return map(ADCReading, 156, 796, 0, 15);
}

void displayIncline(long percentage) {
  lcd.setCursor(0, 0);
  lcd.print(percentage);
  lcd.print("%");

  // Make the trailing % sign of 10% disappear if we drop from 10% to 9%
  // We don't need to worry about 100%, the max is 15%.
  if (percentage < 10) lcd.print(" ");
}

// The Nucleo's onboard button goes low when pressed, so we wrap the logic
// up here instead of repeating it everywhere.
int readButton() {
  return !digitalRead(BUTTON);
}

void setup() {
  Serial.begin(9600);

  Ethernet.begin(mac, localIP);
  connectToMQTT();

  lcd.init();
  lcd.backlight();

  pinMode(ENABLE_ELEV_CHANGE, OUTPUT);
  pinMode(ENABLE_ELEV_READ,   OUTPUT);

  pinMode(RAISE,  OUTPUT);
  pinMode(LOWER,  OUTPUT);
  pinMode(BUTTON, INPUT);

  digitalWrite(ENABLE_ELEV_READ, HIGH);
  digitalWrite(ENABLE_ELEV_CHANGE, HIGH);

  buttonState = readButton();
}

void loop() {
  long incline = inclineAsPercentage(analogRead(A1));
  displayIncline(incline);

  if (millis() - lastMqttSendTime >= MQTT_INTERVAL) {
    lastMqttSendTime = millis();

    publish("/elevation", incline);
    mqtt.loop();
  }

  int newState1 = readButton();
  if (newState1 == buttonState) return; // No change

  delay(10);

  int newState2 = readButton();
  if (newState2 != newState1) return; // Just a flutter, not a proper press

  buttonState = newState2;

  if (buttonState) {
    digitalWrite(direction == UP ? RAISE : LOWER, HIGH);
  } else {
    digitalWrite(direction == UP ? RAISE : LOWER, LOW);
    direction = direction == UP ? DOWN : UP;
  }
}