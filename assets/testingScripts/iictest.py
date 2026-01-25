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
