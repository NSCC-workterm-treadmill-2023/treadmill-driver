int16_t inclineAsDegrees(uint16_t ADCReading) {
  return map(ADCReading, 160, 875, 0, 15);
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
