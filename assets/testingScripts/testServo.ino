#include <Servo.h>
Servo head;
#define SERVO_PIN 9
#define FRONT 90

// Basic test Script for servo motors

int RIGHT=FRONT+33;
int LEFT=FRONT-33;

void setup() {
  head.attach(SERVO_PIN);
  head.write(LEFT);
  delay(1000);
  head.write(RIGHT);
  delay(1000);
  head.write(FRONT);
}

void loop() {
  
}
