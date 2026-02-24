// This will be the "main" file that will drive all movement of the figure

// libraries
#include <Servo.h>
#include <EEPROM.h>  

// Servo pin definitions
#define SERVO_PIN_1 6 // base servo
#define SERVO_PIN_0 7 // head servo
#define SERVOS 10 // wings servo
#define BUTTON 9 // button
#define JOYSTICK_BUTTON_1 4 // angry mode (joystick press)
#define JOYSTICK_BUTTON_2 5 // shaking head (joystick press)

#define FRONT 90
#define MIN_ANGLE 0
#define MAX_ANGLE 180

Servo baseServo; // Base servo
Servo headServo; // Head servo
Servo wing; // Wings servo

int y0Pin = A0;  // Y-axis for base rotation
int y1Pin = A2;  // Y-axis for head rotation

// Read previous positions from EEPROM, ensure valid range for head and base servos
int currentrot0 = constrain(EEPROM.read(0), MIN_ANGLE, MAX_ANGLE);
int currentrot1 = constrain(EEPROM.read(1), MIN_ANGLE, MAX_ANGLE);

int ANGLE = 90; // Change this integer to control the range of movement of wings
int RIGHT=FRONT+ANGLE; 
int LEFT=FRONT-ANGLE; 
float speedMultiplier = 0.8; // Increasing this value slows down the movement of wings, decreasing speeds up the movement
int value;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; 

// Joystick press check - modify this according to your hardware setup
int buttonState1 = 0;  // Variable to store the button state for angry mode.
int buttonState2 = 0; // Variable to store the button state for head shaking mode.

void setup() {
  baseServo.attach(SERVO_PIN_0); // Base servo attach
  headServo.attach(SERVO_PIN_1); // Head servo attach

  baseServo.write(currentrot0); // Setup Base Servo to last known position
  headServo.write(currentrot1); // Setup Head Servo to last known position

  wing.attach(SERVOS); // Wings attach
  wing.write(FRONT);
  moveServoWithProfile(wing, RIGHT, FRONT); // Move dynamically to FRONT
  pinMode(BUTTON, INPUT); // Initialization sensor pin
  pinMode(JOYSTICK_BUTTON_1, INPUT); // Joystick button pin setup
  pinMode(JOYSTICK_BUTTON_2, INPUT); // Joystick button for head shaking
  digitalWrite(BUTTON, HIGH); // Activation of internal pull-up resistor
  digitalWrite(JOYSTICK_BUTTON_1, HIGH); // Activation of internal pull-up resistor for joystick button
  digitalWrite(JOYSTICK_BUTTON_2, HIGH); // Activation of internal pull-up resistor for the new joystick button
}

void loop() {
  
  // Check if joystick button is pressed
  buttonState1 = digitalRead(JOYSTICK_BUTTON_1);
  buttonState2 = digitalRead(JOYSTICK_BUTTON_2);

  if (buttonState1 == LOW) {
    // Joystick is pressed, activate Angry Mode
    activateAngryMode();
  } else {
    // DRIVES JOYSTICKS TO CONTROL BASE AND HEAD SERVOS
    int ROTBASESPEED = 1;
    int ROTHEADSPEED = 1;
  
    int y0Value = analogRead(y0Pin);  // Read joystick for base
    int y1Value = analogRead(y1Pin);  // Read joystick for head

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
  }

  // Check if the new joystick button is pressed (for "no" motion)
  if (buttonState2 == LOW) {
    // Joystick button 2 is pressed, initiate head shaking
    headShakeMotion();
  }

  // DRIVES BUTTON AND WING SERVOS (same as existing code)
  int reading = digitalRead(BUTTON);
  if (reading == LOW) {
    unsigned long currentTime = millis();
    if (currentTime - lastDebounceTime > debounceDelay) {
      flapWings(0.8); // Normal wing flapping speed
      lastDebounceTime = currentTime;
    }
  }
}

void flapWings(int speed) {
  speedMultiplier = speed;
  moveServoWithProfile(wing, FRONT, RIGHT); // Move dynamically to RIGHT
  delay(100);
  moveServoWithProfile(wing, RIGHT, FRONT); // Move dynamically to FRONT
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

void activateAngryMode() {
  unsigned long startTime = millis();  // Store the start time
  unsigned long duration = 4000;       // Angry mode runs for 5000 milliseconds (5 seconds)

  // Run actions for 4 seconds
  while (millis() - startTime < duration) {
    // Make the wings flap faster
    flapWings(0.4); // Faster speed for angry mode
  
    // Rotate the base slightly for angry mode
    currentrot0 += 10;  // Rotate the base a little bit
    currentrot0 = constrain(currentrot0, MIN_ANGLE, MAX_ANGLE);
    baseServo.write(currentrot0);  // Apply new base position
  
    //// Optionally, rotate the head a little as well
    //currentrot1 += 5;  // Rotate the head slightly
    //currentrot1 = constrain(currentrot1, MIN_ANGLE, MAX_ANGLE);
    //headServo.write(currentrot1);  // Apply new head position
    
    delay(100); // Short delay to make it noticeable
  }

  // After 4 seconds, return to normal wing flapping speed
  flapWings(0.8);  // Normal speed
}

void headShakeMotion() {
  // Shake the head back and forth like saying "no"
  int shakeSpeed = 10; // Adjust this value to control the speed of the shake
  int shakeAmount = 40; // How far the head moves

  for (int i = 0; i < 5; i++) {  // Repeat 5 times (you can adjust this)
    currentrot1 -= shakeAmount;
    currentrot1 = constrain(currentrot1, MIN_ANGLE, MAX_ANGLE);
    headServo.write(currentrot1);
    delay(shakeSpeed);  // Delay for smooth motion

    currentrot1 += shakeAmount;
    currentrot1 = constrain(currentrot1, MIN_ANGLE, MAX_ANGLE);
    headServo.write(currentrot1);
    delay(shakeSpeed);  // Delay for smooth motion
  }
}