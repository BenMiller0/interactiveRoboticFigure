// This will be the "main" file that will drive all movement of the figure

// libraries
#include <Servo.h>
#include <EEPROM.h>  

// Servo ping definitions
#define SERVO_PIN_1 6 // base servo
#define SERVO_PIN_0 7 // head servo


#define FRONT 90
#define MIN_ANGLE 0
#define MAX_ANGLE 180

Servo baseServo; // Base servo
Servo headServo; // Head servo
int y0Pin = A0;  // Y-axis for base rotation
int y1Pin = A2;  // Y-axis for head rotation

// Read previous positions from EEPROM, ensure valid range for head and base servos
int currentrot0 = constrain(EEPROM.read(0), MIN_ANGLE, MAX_ANGLE);
int currentrot1 = constrain(EEPROM.read(1), MIN_ANGLE, MAX_ANGLE);

void setup() {
  baseServo.attach(SERVO_PIN_0); // Base servo attach
  headServo.attach(SERVO_PIN_1); // Head servo attach

  baseServo.write(currentrot0); // Setup Base Servo to last known position
  headServo.write(currentrot1); // Setup Head Servo to last known position
}

void loop() {
  

  // -- START --
  // DRIVES JOYSTICKS TO CONTROLL BASE AND HEAD SERVO 
  int y0Value = analogRead(y0Pin);  // Read joystick for base
  int y1Value = analogRead(y1Pin);  // Read joystick for head
  // Dead zone to prevent jitter
  if (y0Value > 562 && currentrot0 < MAX_ANGLE) {       
    currentrot0 += 3; // Instant fast movement
    currentrot0 = min(currentrot0, MAX_ANGLE);
  } else if (y0Value < 462 && currentrot0 > MIN_ANGLE) { 
    currentrot0 -= 3;
    currentrot0 = max(currentrot0, MIN_ANGLE);
  }

  if (y1Value > 562 && currentrot1 < MAX_ANGLE) {       
    currentrot1 += 3;
    currentrot1 = min(currentrot1, MAX_ANGLE);
  } else if (y1Value < 462 && currentrot1 > MIN_ANGLE) { 
    currentrot1 -= 3;
    currentrot1 = max(currentrot1, MIN_ANGLE);
  }

  baseServo.write(currentrot0);
  headServo.write(currentrot1);

  EEPROM.update(0, currentrot0);
  EEPROM.update(1, currentrot1);

  delay(5); // Small delay for smoother motion
  // -- END --

  // -- START --
  // Drives button and wing servos


  // -- END --

}
