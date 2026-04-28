#include <Ethernet.h>
#include <MQTTClient.h>
#include <stdint.h>

// MQTT setup
#define MQTT_CLIENT_NAME "treadmill_controller"
#define MQTT_INTERVAL 100
#define TREADMILL_ID "T9800-1"

// Pin definitions
#define ENABLE_ELEV_CHANGE 2
#define RAISE 3
#define LOWER 4
#define ENABLE_ELEV_READ 7
#define ELEV_READ A1
#define SPEED_CHANGE 6
#define SPEED_READ 5
#define REED_SWITCH_PIN 9

const uint16_t INCLINE_ADC_ZERO = 185;
volatile bool inclineRequested = false;
uint16_t desiredIncline = 185;

#define SPEED_SENSOR_BUFFER_SIZE 10
#define INCLINE_TOLERANCE_ADC 10
volatile uint32_t speedSensorChangeTimes[SPEED_SENSOR_BUFFER_SIZE] = {0};
volatile uint8_t speedSensorIndex = 0;

EthernetClient ethClient;
MQTTClient mqtt = MQTTClient(256);
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress localIP(192, 168, 5, 2);
IPAddress brokerIP(192, 168, 5, 1);
uint32_t lastMqttSendTime = 0;

volatile bool magnetConnected = false;
bool lastMagnetState = true;

void reedSwitchInterruptHandler() {
  bool currentState = digitalRead(REED_SWITCH_PIN);
  magnetConnected = (currentState == HIGH);
  analogWrite(SPEED_CHANGE, 0);
}

void subscribe(const char *topicSuffix) {
  String topic = "/";
  topic.concat(TREADMILL_ID);
  topic.concat(topicSuffix);
  mqtt.subscribe(topic);
}

void publish(const char *topicSuffix, const char *message) {
  String topic = "/";
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

int16_t inclineAsDegrees(uint16_t ADCReading) {
  return map(ADCReading, 160, 875, 0, 15);
}

/* Takes speed in km/h as input, and outputs a value for use with analogWrite()
 *
 * The numbers used in these calculations were decided on by measuring the
 * belt's rotation time at various speeds with a stopwatch, and using desmos
 * to find the line of best fit.
 *
 * https://help.desmos.com/hc/en-us/articles/4406972958733-Regressions */
uint8_t speedToPWMSignal(float speed) {
  float result = 7.680178 * speed + 3.19791;
  if (result <= 0) return 0;
  if (result > 255) return 255;
  return (uint8_t)result;
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
float periodToSpeed(uint32_t period) {
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
  Serial.println(speed);
}

void receive(String &topic, String &payload) {
  if (topic.endsWith("/control/elevation")) {
    uint16_t elevation = (uint16_t)payload.toInt();
    desiredIncline = elevation;
    inclineRequested = true;

  } else if (topic.endsWith("/control/speed")) {
    float speed = payload.toFloat();
    speed = constrain(speed, 0, 24);

    if (digitalRead(REED_SWITCH_PIN) == HIGH) {
      setSpeed(speed);
    }
  }
}

void connectToMQTT() {
  mqtt.begin(brokerIP, 1883, ethClient);
  Serial.println("connecting to broker...");
  while (!mqtt.connect(MQTT_CLIENT_NAME)) {
    delay(100);
  }
  Serial.println("connected to broker.");
  if (!mqtt.connected()) return;
  mqtt.onMessage(receive);
  subscribe("/control/elevation");
  subscribe("/control/speed");
}

void setup() {
  Serial.begin(115200);
  Ethernet.begin(mac, localIP);

  pinMode(ENABLE_ELEV_CHANGE, OUTPUT);
  pinMode(ENABLE_ELEV_READ, OUTPUT);
  pinMode(RAISE, OUTPUT); 
  pinMode(LOWER, OUTPUT);
  pinMode(SPEED_CHANGE, OUTPUT);
  pinMode(SPEED_READ, INPUT);
  pinMode(REED_SWITCH_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SPEED_READ), speedSensorInterruptHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(REED_SWITCH_PIN), reedSwitchInterruptHandler, CHANGE);

  connectToMQTT();

  digitalWrite(ENABLE_ELEV_READ, HIGH);
  digitalWrite(ENABLE_ELEV_CHANGE, HIGH);

  publish("/control/elevation", INCLINE_ADC_ZERO);  //Set elevation to 0 on startup

  Serial.println("System Initialized");

  bool currentPinState = digitalRead(REED_SWITCH_PIN);
  magnetConnected = (currentPinState == HIGH); 
  lastMagnetState = currentPinState;
}

void changeIncline() {
  if (inclineRequested == false) return;

  uint16_t currentIncline = (uint16_t)analogRead(ELEV_READ);
  uint16_t high_range = (currentIncline <= 1023 - INCLINE_TOLERANCE_ADC) ? currentIncline + INCLINE_TOLERANCE_ADC : 1023;
  uint16_t low_range = (currentIncline >= INCLINE_TOLERANCE_ADC) ? currentIncline - INCLINE_TOLERANCE_ADC : 0;

  if (desiredIncline > high_range) {
    Serial.print("RAISING: ");
    Serial.println(currentIncline);
    digitalWrite(LOWER, LOW);
    digitalWrite(RAISE, HIGH);
  } else if (desiredIncline < low_range) {
    Serial.print("LOWERING: ");
    Serial.println(currentIncline);
    digitalWrite(RAISE, LOW);
    digitalWrite(LOWER, HIGH);
  } else {
    Serial.println("No Move");
    digitalWrite(RAISE, LOW);
    digitalWrite(LOWER, LOW);
    inclineRequested = false;
  }
}

void loop() {
  // Emergency Message Topics Publish
  bool currentState = digitalRead(REED_SWITCH_PIN);
  if (currentState != lastMagnetState) {
    if (lastMagnetState == LOW && currentState == HIGH) {
    // connected state
      magnetConnected = true; // Allow speed again
      publish("/emergency", "Reed switch reconnected - safe to operate");

    } else if (lastMagnetState == HIGH && currentState == LOW) {
    // disconnected state
      magnetConnected = true; // Allow speed again
      publish("/emergency", "Reed switch disconnected - unsafe to operate");

    }
    lastMagnetState = currentState;
  }

  

  if (!mqtt.connected()) connectToMQTT();

  mqtt.loop();
  changeIncline();

  if (millis() - lastMqttSendTime >= MQTT_INTERVAL) {
    lastMqttSendTime = millis();

    publish("/readings/elevation", analogRead(ELEV_READ));

    // Disable interupts to prevent race condition while reading speed values
    noInterrupts();
    uint8_t idx = speedSensorIndex;
    uint32_t newest = speedSensorChangeTimes[idx];
    uint32_t oldest = speedSensorChangeTimes[(idx + 1) % SPEED_SENSOR_BUFFER_SIZE];
    interrupts();
    
    // prevent erroneous startup values where the buffer is 0s
    if(oldest == 0) {
      publish("/readings/speed", 0.0f);
    } else {
      uint32_t averagePeriod = (newest - oldest) / (SPEED_SENSOR_BUFFER_SIZE - 1);
      publish("/readings/speed", periodToSpeed(averagePeriod));
    }
  }
}
