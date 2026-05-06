#include <Ethernet.h>
#include <MQTTClient.h>
#include <stdint.h>

// MQTT setup
#define MQTT_CLIENT_NAME "treadmill_controller"
#define MQTT_PORT 1883
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

// Safety state enum
enum SafetyStatus { SAFE, UNSAFE };

EthernetClient ethClient;
MQTTClient mqtt = MQTTClient(256);
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress localIP(192, 168, 5, 2);
IPAddress brokerIP(192, 168, 5, 1);
uint32_t lastMqttSendTime = 0;

volatile SafetyStatus safetyState = SAFE;  // Default to safe state until ISR is re-enabled
volatile bool safetyStateChanged = false;  // Flag set by ISR when switch state changes



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

  publish("/control/elevation", (long)INCLINE_ADC_ZERO);  //Set elevation to 0 on startup

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
  //     publish("/emergency", "Reed switch reconnected - safe to operate");
  //   } else {
  //     publish("/emergency", "Reed switch disconnected - unsafe to operate");
  //   }
  // }

  if (!mqtt.connected()) connectToMQTT(brokerIP, ethClient);

  mqtt.loop();
  changeIncline();

  if (millis() - lastMqttSendTime >= MQTT_INTERVAL) {
    lastMqttSendTime = millis();

    publish("/readings/elevation", (long)analogRead(ELEV_READ));

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
