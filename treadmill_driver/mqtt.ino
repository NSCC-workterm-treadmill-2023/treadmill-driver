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

void receive(String &topic, String &payload) {
  if (topic.endsWith("/control/elevation")) {
    uint16_t elevation = (uint16_t)payload.toInt();
    desiredIncline = elevation;
    inclineRequested = true;

  } else if (topic.endsWith("/control/speed")) {
    float speed = payload.toFloat();
    speed = constrain(speed, 0, 24);

    if (safetyState == SAFE) {
      setSpeed(speed);
    }
  }
}

void connectToMQTT(IPAddress brokerIP, EthernetClient &ethClient) {
  mqtt.begin(brokerIP, MQTT_PORT, ethClient);
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
