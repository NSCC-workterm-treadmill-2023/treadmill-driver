# Treadmill Protocol

This interfaces directly with the treadmill connector, which would ordinarily connect the front panel up top to the motor controller
in the bottom. The treadmill connector has a notch on top. Pin 1 is top left, pin 2 is under pin 1, pin 3 is to the right of pin 1, pin 4 is
under pin 2, and so on.

## Elevation

### Reading elevation

The elevation sensor is a potentiometer. Put power on pin 5 and ground on pin 3, then read the analog signal on pin 4. Note that the pot's
ground is not connected to any other ground - you should do that yourself. For a 3.3V input, the minimum value is 0.8V, representing a flat
treadmill. The maximum value is 3.12V, representing the steepest incline possible.

### Changing elevation

Hold pin 15 high to warm up the motors and enable elevation changes. Then hold pin 14 high to increase the elevation, **or** hold pin 13 high
to decrease the elevation. The motor controller has safeguards in place, so you can't go past the minimum and maximum values.

## Speed

### Reading speed

The speed sensor senses a rotating cog attached to the motor. A magnetic sensor is placed in line with the cogs. When a cog is in front of a sensor,
the line goes high. When there's a gap in front of the sensor, the line goes low (or maybe it's vice versa). You can deduce the speed by measuring
the time between changes in the signal. Read the signal on pin 6.

### Changing speed

This one's simple: just put a PWM signal on pin 9. I think you need to hold pin 15 high for this to work, but I haven't tried without it.