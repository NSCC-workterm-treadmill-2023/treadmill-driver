void reedSwitchInterruptHandler() {
  // TODO: Uncomment below to re-enable switch-driven safetyState control
  safetyState = (digitalRead(REED_SWITCH_PIN) == HIGH) ? SAFE : UNSAFE;
  safetyStateChanged = true;
  analogWrite(SPEED_CHANGE, 0);
}
