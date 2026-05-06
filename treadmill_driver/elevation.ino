int16_t inclineAsDegrees(uint16_t ADCReading) {
  return map(ADCReading, INCLINE_ADC_MIN, INCLINE_ADC_MAX, 0, 15);
}

void changeIncline(uint16_t targetIncline) {
  if (inclineRequested == false) return;

  uint16_t currentIncline = (uint16_t)analogRead(ELEV_READ);
  uint16_t highRange = (currentIncline <= 1023 - INCLINE_TOLERANCE_ADC) ? currentIncline + INCLINE_TOLERANCE_ADC : 1023;
  uint16_t lowRange = (currentIncline >= INCLINE_TOLERANCE_ADC) ? currentIncline - INCLINE_TOLERANCE_ADC : 0;

  if (targetIncline > highRange) {
    Serial.print("RAISING: ");
    Serial.println(currentIncline);
    digitalWrite(LOWER, LOW);
    digitalWrite(RAISE, HIGH);
  } else if (targetIncline < lowRange) {
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
