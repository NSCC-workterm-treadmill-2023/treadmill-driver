void reedSwitchInterruptHandler() {
  safetyState = (digitalRead(REED_SWITCH_PIN) == HIGH) ? SAFE : UNSAFE;
  safetyStateChanged = true;
  analogWrite(SPEED_CHANGE, 0);
}
