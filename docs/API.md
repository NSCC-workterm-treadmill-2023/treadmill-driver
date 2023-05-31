# Treadmill API

The treadmill communicates over MQTT. The treadmill has a unique identifier: **T9800-1**. All MQTT topics for the treadmill are prefixed with
/T9800-1. That allows us to control multiple treadmills from a single MQTT broker later, if we desire.

## Elevation

Subscribe to /{id}/readings/elevation to get the current elevation. This broadcasts an integer from 0 to 15, where 0 is flat and 15 is the
maximum incline.

Publish to /{id}/control/elevation to change the current elevation. This accepts an integer from 0 to 15, where 0 is flat and 15 is the
maximum incline. Numbers less than 0 will be treated as 0. Numbers greater than 15 will be treated as 15.

## Speed

Subscribe to /{id}/readings/speed to get the current speed as a float in km/h.

Publish to /{id}/control/speed to change the current speed.

Values below 0 will be treated as 0. Values above 24 will be treated as 24.