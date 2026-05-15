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
  float speed = 13094.3 / ((float) period + 169.358) - 0.204098;
  return speed < 0 ? 0.0f : speed;
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
