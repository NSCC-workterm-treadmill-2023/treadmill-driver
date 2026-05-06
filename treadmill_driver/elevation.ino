int16_t inclineAsDegrees(uint16_t ADCReading) {
  return map(ADCReading, INCLINE_ADC_MIN, INCLINE_ADC_MAX, 0, 15);
}

void changeIncline(uint16_t targetIncline) {
  if (inclineRequested == false) return;

  uint16_t currentIncline = (uint16_t)analogRead(ELEV_READ);
  uint16_t highRange = (currentIncline <= 1023 - INCLINE_TOLERANCE_ADC) ? currentIncline + INCLINE_TOLERANCE_ADC : 1023;
  uint16_t lowRange = (currentIncline >= INCLINE_TOLERANCE_ADC) ? currentIncline - INCLINE_TOLERANCE_ADC : 0;

  if (targetIncline > highRange) {
    // Stall detection for RAISE direction
    if (stallCheckTime == 0) {
      // Initialize stall checkpoint
      stallCheckTime = millis();
      stallCheckADC = currentIncline;
    } else if (abs((int16_t)currentIncline - (int16_t)stallCheckADC) >= STALL_MOVEMENT_THRESHOLD) {
      // Motor is moving, reset checkpoint
      stallCheckTime = millis();
      stallCheckADC = currentIncline;
    } else if (millis() - stallCheckTime >= INCLINE_STALL_TIMEOUT_MS) {
      // Stall detected: ADC hasn't moved enough in 500ms
      Serial.println("STALL DETECTED - RAISING");
      digitalWrite(RAISE, LOW);
      digitalWrite(LOWER, LOW);
      inclineRequested = false;
      stallCheckTime = 0;
      return;
    }

    Serial.print("RAISING: ");
    Serial.println(currentIncline);
    digitalWrite(LOWER, LOW);
    digitalWrite(RAISE, HIGH);

  } else if (targetIncline < lowRange) {
    // Stall detection for LOWER direction
    if (stallCheckTime == 0) {
      // Initialize stall checkpoint
      stallCheckTime = millis();
      stallCheckADC = currentIncline;
    } else if (abs((int16_t)currentIncline - (int16_t)stallCheckADC) >= STALL_MOVEMENT_THRESHOLD) {
      // Motor is moving, reset checkpoint
      stallCheckTime = millis();
      stallCheckADC = currentIncline;
    } else if (millis() - stallCheckTime >= INCLINE_STALL_TIMEOUT_MS) {
      // Stall detected: ADC hasn't moved enough in 500ms
      Serial.println("STALL DETECTED - LOWERING");
      digitalWrite(RAISE, LOW);
      digitalWrite(LOWER, LOW);
      inclineRequested = false;
      stallCheckTime = 0;
      return;
    }

    Serial.print("LOWERING: ");
    Serial.println(currentIncline);
    digitalWrite(RAISE, LOW);
    digitalWrite(LOWER, HIGH);

  } else {
    Serial.println("No Move");
    digitalWrite(RAISE, LOW);
    digitalWrite(LOWER, LOW);
    inclineRequested = false;
    stallCheckTime = 0;
    stallCheckADC = 0;
  }
}
