#include <Servo.h>

Servo servo1;  // Servo on pin 2
Servo servo2;  // Servo on pin 3

void setup() {
    servo1.attach(2);  // Attach servo1 to pin 2
    servo2.attach(3);  // Attach servo2 to pin 3
}

void loop() {
    // Move servos in opposite directions (full range)
    for (int pos = 0; pos <= 180; pos += 3) {  // Smoother, slightly slower
        servo1.write(pos);       // Move forward
        servo2.write(180 - pos); // Move backward
        delay(10);  // Slightly slower movement
    }

    // Move servos back to starting position
    for (int pos = 180; pos >= 0; pos -= 3) {
        servo1.write(pos);       // Move back
        servo2.write(180 - pos); // Move back
        delay(10);
    }
}
