# Treadmill Protocol

This interfaces directly with the treadmill connector, which would ordinarily connect the front panel up top to the motor controller
in the bottom. The treadmill connector has a notch on top. Pin 1 is top left, pin 2 is under pin 1, pin 3 is to the right of pin 1, pin 4 is
under pin 2, and so on.

## Elevation

### Reading elevation

Hold pin 5 high to enable readings. The elevation will be an analog signal on pin 4. The minimum value is 0.8V, representing a flat treadmill.
The maximum value is 3.12V, representing the steepest incline possible.

### Changing elevation

Hold pin 15 high to warm up the motors and enable elevation changes. Then hold pin 14 high to increase the elevation, **or** hold pin 13 high
to decrease the elevation. The motor controller has safeguards in place, so you can't go past the minimum and maximum values.