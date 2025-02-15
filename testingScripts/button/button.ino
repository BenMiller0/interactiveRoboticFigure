#include <Servo.h>
#include <EEPROM.h>
Servo wing;

#define SERVOS 10
#define BUTTON 9

#define FRONT 50
int ANGLE = 90; // Change this integer to control the range of movement
int RIGHT=FRONT+ANGLE; 
int LEFT=FRONT-ANGLE; 

float speedMultiplier = 0.8; // Increasing this value from slows down the movement, decreasing the value speeds up the movement

int value;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; 

void setup() {
  wing.attach(SERVOS);
  wing.write(FRONT);
  delay(1000);

  moveServoWithProfile(wing, FRONT, RIGHT); // Move dynamically to RIGHT
  delay(100);
  moveServoWithProfile(wing, RIGHT, FRONT); // Move dynamically to FRONT

  pinMode(BUTTON, INPUT); // Initialization sensor pin
  digitalWrite(BUTTON, HIGH); // Activation of internal pull-up resistor
  Serial.begin(9600); // Initialization of the serial monitor
  Serial.println("KY-004 Button test");
}

void loop() {
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