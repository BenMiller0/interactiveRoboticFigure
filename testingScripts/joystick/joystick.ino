#include <Servo.h>
#include <EEPROM.h>  

#define SERVO_PIN_0 7
#define SERVO_PIN_1 6
#define FRONT 90
#define MIN_ANGLE 0
#define MAX_ANGLE 180

Servo baseServo;
Servo headServo;
int y0Pin = A0;  // Y-axis for base rotation
int y1Pin = A2;  // Y-axis for head rotation

// Read previous positions from EEPROM, ensure valid range
int currentrot0 = constrain(EEPROM.read(0), MIN_ANGLE, MAX_ANGLE);
int currentrot1 = constrain(EEPROM.read(1), MIN_ANGLE, MAX_ANGLE);

void setup() {
  baseServo.attach(SERVO_PIN_0);
  headServo.attach(SERVO_PIN_1);

  // Move servos to their last known positions
  baseServo.write(currentrot0);
  headServo.write(currentrot1);
}

void loop() {
  int y0Value = analogRead(y0Pin);  // Read joystick/sensor for base
  int y1Value = analogRead(y1Pin);  // Read joystick/sensor for head

  // Dead zone to prevent jitter (adjust 50 if needed)
  if (y0Value > 562 && currentrot0 < MAX_ANGLE) {       
    ++currentrot0;
  } else if (y0Value < 462 && currentrot0 > MIN_ANGLE) { 
    --currentrot0;
  }

  if (y1Value > 562 && currentrot1 < MAX_ANGLE) {       
    ++currentrot1;
  } else if (y1Value < 462 && currentrot1 > MIN_ANGLE) { 
    --currentrot1;
  }

  // Move servos to updated positions
  baseServo.write(currentrot0);
  headServo.write(currentrot1);

  // Save positions to EEPROM only if they changed
  EEPROM.update(0, currentrot0);
  EEPROM.update(1, currentrot1);

  delay(5);  // Small delay to smooth movement
}
