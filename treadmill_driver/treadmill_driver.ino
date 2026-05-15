#include <Ethernet.h>
#include <MQTTClient.h>
#include <stdint.h>

// MQTT setup
#define MQTT_CLIENT_NAME "treadmill_controller"
#define MQTT_PORT 1883
#define TREADMILL_ID "T9800-1"

// MQTT topics
#define TOPIC_CONTROL_ELEVATION "/control/elevation"
#define TOPIC_CONTROL_SPEED "/control/speed"
#define TOPIC_READINGS_ELEVATION "/readings/elevation"
#define TOPIC_READINGS_SPEED "/readings/speed"
#define TOPIC_EMERGENCY "/emergency"

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
uint16_t desiredIncline = 210;

#define SPEED_SENSOR_BUFFER_SIZE 10
#define INCLINE_TOLERANCE_ADC 15
#define INCLINE_ADC_MIN 210
#define INCLINE_ADC_MAX 800
#define SPEED_MIN 0
#define SPEED_MAX 7
#define INCLINE_STALL_TIMEOUT_MS 500
#define SPEED_STALL_TIMEOUT_MS 500
#define INCLINE_STALL_MOVEMENT_THRESHOLD 10
#define ELEVATION_PUBLISH_THRESHOLD 10
#define SPEED_PUBLISH_THRESHOLD 0.1
volatile uint32_t speedSensorChangeTimes[SPEED_SENSOR_BUFFER_SIZE] = {0};
volatile uint8_t speedSensorIndex = 0;

// Safety state enum
enum SafetyStatus { SAFE, UNSAFE };

EthernetClient ethClient;
MQTTClient mqtt = MQTTClient(256);
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress localIP(192, 168, 5, 2);
IPAddress brokerIP(192, 168, 5, 1);

uint16_t lastPublishedElevation = 0;
float lastPublishedSpeed = -1.0f;

volatile SafetyStatus safetyState = SAFE;  // Default to safe state until ISR is re-enabled
volatile bool safetyStateChanged = false;  // Flag set by ISR when switch state changes

uint32_t stallCheckTime = 0;
uint16_t stallCheckADC = 0;



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

  connectToMQTT(brokerIP, ethClient);

  digitalWrite(ENABLE_ELEV_READ, HIGH);
  digitalWrite(ENABLE_ELEV_CHANGE, HIGH);

  publish(TOPIC_CONTROL_ELEVATION, (long)INCLINE_ADC_ZERO);  //Set elevation to 0 on startup

  Serial.println("System Initialized");

  // TODO: Uncomment below when reed switch hardware is ready
  // safetyState = (digitalRead(REED_SWITCH_PIN) == HIGH) ? SAFE : UNSAFE;
}

void loop() {
  // TODO: Uncomment below when reed switch hardware is ready
  // Emergency Message Topics Publish
  // if (safetyStateChanged) {
  //   safetyStateChanged = false;  // Clear flag so publish happens only once
  //   if (safetyState == SAFE) {
  //     publish(TOPIC_EMERGENCY, "Reed switch reconnected - safe to operate");
  //   } else {
  //     publish(TOPIC_EMERGENCY, "Reed switch disconnected - unsafe to operate");
  //   }
  // }

  if (!mqtt.connected()) connectToMQTT(brokerIP, ethClient);

  mqtt.loop();
  changeIncline(desiredIncline);

  uint16_t currentElevation = (uint16_t)analogRead(ELEV_READ);
  if (abs((int16_t)currentElevation - (int16_t)lastPublishedElevation) >= ELEVATION_PUBLISH_THRESHOLD) {
    lastPublishedElevation = currentElevation;
    publish(TOPIC_READINGS_ELEVATION, (long)currentElevation);
  }

  // Disable interrupts to prevent race condition while reading speed values
  noInterrupts();
  uint8_t idx = speedSensorIndex;
  uint32_t newest = speedSensorChangeTimes[idx];
  uint32_t oldest = speedSensorChangeTimes[(idx + 1) % SPEED_SENSOR_BUFFER_SIZE];
  uint32_t now = micros();
  interrupts();

  float currentSpeed;
  // prevent erroneous startup values where the buffer is 0s
  if (oldest == 0) {
    currentSpeed = 0.0f;
  } else if (now - newest > (uint32_t)SPEED_STALL_TIMEOUT_MS * 1000UL) {
    // No pulse received recently - belt has stopped
    currentSpeed = 0.0f;
  } else {
    uint32_t averagePeriod = (newest - oldest) / (SPEED_SENSOR_BUFFER_SIZE - 1);
    currentSpeed = periodToSpeed(averagePeriod);
  }

  if (fabs(currentSpeed - lastPublishedSpeed) >= SPEED_PUBLISH_THRESHOLD) {
    lastPublishedSpeed = currentSpeed;
    publish(TOPIC_READINGS_SPEED, currentSpeed);
  }
}
