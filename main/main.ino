// This will be the "main" file that will drive all movement of the figure

// libraries
#include <Servo.h>
#include <EEPROM.h>

// Servo pin definitions
#define SERVO_PIN_1 6 // base servo
#define SERVO_PIN_0 7 // head servo
#define WING1 10      // Right wing servo
#define WING2 11      // Left wing servo
#define BUTTON 9      // Button
#define JOYSTICK_BUTTON_1 4 // Angry mode (joystick press)
#define JOYSTICK_BUTTON_2 5 // Shaking head (joystick press)

#define FRONT 90
#define MIN_ANGLE 0
#define MAX_ANGLE 180

Servo baseServo; // Base servo
Servo headServo; // Head servo
Servo wing1;     // Right wing servo
Servo wing2;     // Left wing servo

int y0Pin = A0; // Y-axis for base rotation
int y1Pin = A2; // Y-axis for head rotation

// Read previous positions from EEPROM, ensure valid range for head and base servos
int currentrot0 = constrain(EEPROM.read(0), MIN_ANGLE, MAX_ANGLE);
int currentrot1 = constrain(EEPROM.read(1), MIN_ANGLE, MAX_ANGLE);

int ANGLE = 90; // Change this integer to control the range of movement of wings
int UP = FRONT + ANGLE;
int DOWN = FRONT - ANGLE;
float speedMultiplier = 0.8; // Increasing slows down the movement of wings, decreasing speeds it up
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Joystick press check - modify this according to your hardware setup
int buttonState1 = 0; // Angry mode
int buttonState2 = 0; // Head shaking mode

void setup() {
  baseServo.attach(SERVO_PIN_0);
  headServo.attach(SERVO_PIN_1);
  
  baseServo.write(currentrot0);
  headServo.write(currentrot1);

  wing1.attach(WING1);
  wing2.attach(WING2);
  
  wing1.write(FRONT);
  wing2.write(FRONT);
  
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(JOYSTICK_BUTTON_1, INPUT_PULLUP);
  pinMode(JOYSTICK_BUTTON_2, INPUT_PULLUP);
}

void loop() {
  // Check joystick button for Angry Mode
  buttonState1 = digitalRead(JOYSTICK_BUTTON_1);
  buttonState2 = digitalRead(JOYSTICK_BUTTON_2);

  if (buttonState1 == LOW) {
    activateAngryMode();
  } else {
    driveBaseAndHead();
  }

  // Check joystick button for head shake
  if (buttonState2 == LOW) {
    headShakeMotion();
  }

  // Check button for wing movement
  if (digitalRead(BUTTON) == LOW) {
    unsigned long currentTime = millis();
    if (currentTime - lastDebounceTime > debounceDelay) {
      flapWings(1); // Normal wing flapping speed
      lastDebounceTime = currentTime;
    }
  }
}

void driveBaseAndHead() {
  int ROTBASESPEED = 1;
  int ROTHEADSPEED = 1;
  
  int y0Value = analogRead(y0Pin);
  int y1Value = analogRead(y1Pin);

  if (y0Value > 562 && currentrot0 < MAX_ANGLE) {       
    currentrot0 += ROTBASESPEED;
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

  delay(5);
}

void flapWings(int speed) {
    float speedMultiplier = speed; // Make sure to declare speedMultiplier

    // Wings flap up together
    int currRot = FRONT;
    for (int i = 0; currRot + i < 500; i += speed) {
        wing1.write(currRot + i);
        wing2.write(currRot - i);
        delay(2);
    }

      wing1.write(FRONT);
      wing2.write(FRONT);

    delay(100 * speedMultiplier);
}

void moveServoWithProfile(Servo &servo, int startPos, int endPos) {
  float profile[] = {0.0, 0.01, 0.03, 0.06, 0.10, 0.15, 0.21, 0.28, 0.36, 0.44,
                     0.52, 0.60, 0.68, 0.75, 0.82, 0.88, 0.93, 0.97, 1.0};
  int steps = sizeof(profile) / sizeof(profile[0]);
  int baseDelay = 20;

  for (int i = 0; i < steps; i++) {
    int pos = startPos + (int)((endPos - startPos) * profile[i]);
    servo.write(pos);
    delay((int)(baseDelay * speedMultiplier));
  }
}

void activateAngryMode() {
  unsigned long startTime = millis();
  unsigned long duration = 4000;

  while (millis() - startTime < duration) {
    flapWings(3);


    delay(100);
  }

}

void headShakeMotion() {
  int shakeSpeed = 10;
  int shakeAmount = 40;

  for (int i = 0; i < 5; i++) {
    currentrot1 -= shakeAmount;
    currentrot1 = constrain(currentrot1, MIN_ANGLE, MAX_ANGLE);
    headServo.write(currentrot1);
    delay(shakeSpeed);

    currentrot1 += shakeAmount;
    currentrot1 = constrain(currentrot1, MIN_ANGLE, MAX_ANGLE);
    headServo.write(currentrot1);
    delay(shakeSpeed);
  }
}
