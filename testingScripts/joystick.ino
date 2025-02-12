#include <Servo.h>
#include <EEPROM.h>  


#define SERVO_PIN 7
#define FRONT 90
#define MIN_ANGLE 0
#define MAX_ANGLE 180


Servo head;
int yPin = A0;  // Y-axis
int RIGHT=FRONT+33;
int LEFT=FRONT-40;
int currentrot = EEPROM.read(0);

void setup() {
  head.attach(SERVO_PIN);
}

void loop() {
  int xValue = analogRead(yPin);  // Read joystick/sensor

  // Optional: Add a dead zone to avoid jitter around the center
  if (xValue > 512 + 50) {       
    if (!(currentrot > MAX_ANGLE)) {
      currentrot++;
    }
  } else if (xValue < 512 - 50) { 
    if (!(currentrot < MIN_ANGLE)) {
      --currentrot;
    }
  }


  head.write(currentrot);
  delay(5);  // Small delay to smooth out movement
}
