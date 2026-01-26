# This is a servo control test using I2C in python
# Moves servos on channel 0 and 1 in their full range

from adafruit_servokit import ServoKit
import time

kit = ServoKit(channels=16)

# Sweep channel 0 servo slowly
for angle in range(0, 181, 10):
    kit.servo[0].angle = angle
    kit.servo[1].angle = angle
    time.sleep(0.5)

for angle in range(180, -1, -10):
    kit.servo[0].angle = angle
    kit.servo[1].angle = angle
    time.sleep(0.5)
