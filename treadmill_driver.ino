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
#define ELEV_READ A1
#define SPEED_CHANGE 6
#define SPEED_READ 5

enum Direction {UP, DOWN};

int buttonState;
int desiredIncline = 0;
bool inclineChangeRequested = false;
int direction = UP;

#define SPEED_SENSOR_BUFFER_SIZE 10
long speedSensorChangeTimes[] = {0, 0, 0, 0 ,0, 0, 0, 0, 0, 0};
unsigned int speedSensorIndex = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

EthernetClient ethClient;
MQTTClient mqtt = MQTTClient(256); // Param is max packet size
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress localIP(192, 168, 5, 2);
IPAddress brokerIP(192, 168, 5, 1);
uint32_t lastMqttSendTime = 0;

void subscribe(const char *topicSuffix) {
  String topic = String("/");
  topic.concat(TREADMILL_ID);
  topic.concat(topicSuffix);

  mqtt.subscribe(topic);
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

void publish(const char *topicSuffix, float message) {
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

void displaySpeedSensorTime(float speed) {
  lcd.setCursor(0, 1);
  lcd.print("                 ");
  lcd.setCursor(0, 1);
  lcd.print(speed);
}

/* Takes speed in km/h as input, and outputs a value for use with analogWrite()
 *
 * The numbers used in these calculations were decided on by measuring the
 * belt's rotation time at various speeds with a stopwatch, and using desmos
 * to find the line of best fit.
 *
 * https://help.desmos.com/hc/en-us/articles/4406972958733-Regressions */
int speedToPWMSignal(float speed) {
  return 7.680178 * speed + 3.19791;
}

/* Takes a period in microseconds as input, and outputs the speed it represents
 * in km/h. Technically, it's only half a period - the distance of a cog or gap
 * in the sensor.
 *
 * As above, we're using desmos to find the curve of best fit - this time it's
 * a reciprocal function, since there's a linear relationship between frequency
 * and speed, but we're measuring period.
 *
 * https://help.desmos.com/hc/en-us/articles/4406972958733-Regressions */
float periodToSpeed(long period) {
  return 13094.3 / ((float) period + 169.358) - 0.204098;
}

/* The speed sensor is built on a rotating cog wheel with a magnetic sensor.
 * When a cog is in front of the sensor, it goes high, and when there's a
 * gap, it goes low (or maybe it's vice versa). By measuring the frequency
 * or period of the changes, we can deduce the speed.
 *
 * The sensor triggers frequently - multiple times per loop iteration. So we
 * add a buffer, and store the last SPEED_SENSOR_BUFFER_SIZE readings in it.
 * Rather than keeping the buffer array sorted, we simply store the index of
 * the most recent reading. */
void speedSensorInterruptHandler() {
  speedSensorIndex = (speedSensorIndex + 1) % SPEED_SENSOR_BUFFER_SIZE;
  speedSensorChangeTimes[speedSensorIndex] = micros();
}

void setSpeed(float speed) {
  analogWrite(SPEED_CHANGE, speedToPWMSignal(speed));
}

// The Nucleo's onboard button goes low when pressed, so we wrap the logic
// up here instead of repeating it everywhere.
int readButton() {
  return !digitalRead(BUTTON);
}

void receive(String &topic, String &payload) {
  if (topic.endsWith("/control/elevation")) {
    long elevation = payload.toInt();
    if (elevation < 0) elevation = 0;
    else if (elevation > 15) elevation = 15;

    desiredIncline = elevation;
    inclineChangeRequested = true;
  } else if (topic.endsWith("/control/speed")) {
    float speed = payload.toFloat();
    if (speed > 24) speed = 24;
    else if (speed < 0) speed = 0;

    setSpeed(speed);
  }
}

void connectToMQTT() {
  mqtt.begin(brokerIP, 1883, ethClient);

  while (!mqtt.connect(MQTT_CLIENT_NAME)) {
    delay(100);
  }

  if (!mqtt.connected()) {
    return;
  }

  mqtt.onMessage(receive);
  subscribe("/control/elevation");
  subscribe("/control/speed");
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

  pinMode(SPEED_CHANGE, OUTPUT);
  pinMode(SPEED_READ,   INPUT);

  digitalWrite(ENABLE_ELEV_READ, HIGH);
  digitalWrite(ENABLE_ELEV_CHANGE, HIGH);

  buttonState = readButton();
  attachInterrupt(digitalPinToInterrupt(SPEED_READ), speedSensorInterruptHandler, CHANGE);
}

void changeIncline(long currentIncline) {
  if (!inclineChangeRequested) return;

  if (desiredIncline > currentIncline) {
    digitalWrite(RAISE, HIGH);
  } else if (desiredIncline < currentIncline) {
    digitalWrite(LOWER, HIGH);
  } else {
    digitalWrite(RAISE, LOW);
    digitalWrite(LOWER, LOW);
    inclineChangeRequested = false;
  }
}

void loop() {
  if (!mqtt.connected()) connectToMQTT();

  long currentIncline = inclineAsPercentage(analogRead(ELEV_READ));
  displayIncline(currentIncline);
  changeIncline(currentIncline);

  // Try to keep the LCD from updating too frantically - should be easier to read.
  if (micros() % 10 == 0) {
    displaySpeedSensorTime(
      periodToSpeed(
        speedSensorChangeTimes[speedSensorIndex] - speedSensorChangeTimes[(speedSensorIndex - 1) % SPEED_SENSOR_BUFFER_SIZE]
      )
    );
  }

  if (millis() - lastMqttSendTime >= MQTT_INTERVAL) {
    lastMqttSendTime = millis();

    publish("/readings/elevation", currentIncline);
    publish("/readings/speed", periodToSpeed(
      speedSensorChangeTimes[speedSensorIndex] - speedSensorChangeTimes[(speedSensorIndex - 1) % SPEED_SENSOR_BUFFER_SIZE]
    ));
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