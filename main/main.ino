// This will be the "main" file that will drive all movement of the figure

// libraries
#include <Servo.h>
#include <EEPROM.h>  

// Servo ping definitions
#define SERVO_PIN_1 6 // base servo
#define SERVO_PIN_0 7 // head servo
#define SERVOS 10 // wings servos
#define BUTTON 9 // button

#define FRONT 90
#define MIN_ANGLE 0
#define MAX_ANGLE 180

Servo baseServo; // Base servo
Servo headServo; // Head servo
Servo wing; // Wings Servo

int y0Pin = A0;  // Y-axis for base rotation
int y1Pin = A2;  // Y-axis for head rotation

// Read previous positions from EEPROM, ensure valid range for head and base servos
int currentrot0 = constrain(EEPROM.read(0), MIN_ANGLE, MAX_ANGLE);
int currentrot1 = constrain(EEPROM.read(1), MIN_ANGLE, MAX_ANGLE);

int ANGLE = 90; // Change this integer to control the range of movement of WINGS
int RIGHT=FRONT+ANGLE; 
int LEFT=FRONT-ANGLE; 
float speedMultiplier = 0.8; // Increasing this value from slows down the movement of WINGS, decreasing the value speeds up the movement
int value;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; 

void setup() {
  baseServo.attach(SERVO_PIN_0); // Base servo attach
  headServo.attach(SERVO_PIN_1); // Head servo attach

  baseServo.write(currentrot0); // Setup Base Servo to last known position
  headServo.write(currentrot1); // Setup Head Servo to last known position

  wing.attach(SERVOS); // Wings attach
  wing.write(FRONT);
  moveServoWithProfile(wing, RIGHT, FRONT); // Move dynamically to FRONT
  pinMode(BUTTON, INPUT); // Initialization sensor pin
  digitalWrite(BUTTON, HIGH); // Activation of internal pull-up resistor
}

void loop() {
  

  // -- START --
  // DRIVES JOYSTICKS TO CONTROLL BASE AND HEAD SERVO 
  int ROTBASESPEED = 1; // Speed of base rotation
  int ROTHEADSPEED = 1; // Speed of base rotation

  int y0Value = analogRead(y0Pin);  // Read joystick for base
  int y1Value = analogRead(y1Pin);  // Read joystick for head
  // Dead zone to prevent jitter
  if (y0Value > 562 && currentrot0 < MAX_ANGLE) {       
    currentrot0 += ROTBASESPEED; // Instant fast movement
    currentrot0 = min(currentrot0, MAX_ANGLE);
  } else if (y0Value < 462 && currentrot0 > MIN_ANGLE) { 
    currentrot0 -= ROTBASESPEED;
    currentrot0 = max(currentrot0, MIN_ANGLE);
  }

  if (y1Value > 562 && currentrot1 < MAX_ANGLE) {       
    currentrot1 += ROTHEADSPEED;
    currentrot1 = min(currentrot1, MAX_ANGLE);
  } else if (y1Value < 462 && currentrot1 > MIN_ANGLE) { 
    currentrot1 -= ROTHEADSPEED;
    currentrot1 = max(currentrot1, MIN_ANGLE);
  }

  baseServo.write(currentrot0);
  headServo.write(currentrot1);

  EEPROM.update(0, currentrot0);
  EEPROM.update(1, currentrot1);

  delay(5); // Small delay for smoother motion
  // -- END --

  // -- START --
  // DRIVES BUTTON AND WING SERVOS
   int reading = digitalRead(BUTTON);

  if (reading == LOW) {
    unsigned long currentTime = millis();

    if (currentTime - lastDebounceTime > debounceDelay) {
      moveServoWithProfile(wing, FRONT, RIGHT); // Move dynamically to RIGHT
      delay(100);
      moveServoWithProfile(wing, RIGHT, FRONT); // Move dynamically to FRONT

      // Update the last debounce time
      lastDebounceTime = currentTime;
    }
  }

  // -- END --

}

void moveServoWithProfile(Servo &servo, int startPos, int endPos) {
  // Dynamic speed profile array
  float profile[] = {0.0, 0.01, 0.03, 0.06, 0.10, 0.15, 0.21, 0.28, 0.36, 0.44,
                     0.52, 0.60, 0.68, 0.75, 0.82, 0.88, 0.93, 0.97, 1.0};
  int steps = sizeof(profile) / sizeof(profile[0]);
  // Base delay in milliseconds for each step
  int baseDelay = 20; 

  for (int i = 0; i < steps; i++) {
    int pos = startPos + (int)((endPos - startPos) * profile[i]);
    servo.write(pos);
    // Adjusting baseDelay by speedMultiplier
    delay((int)(baseDelay * speedMultiplier));
  }
}