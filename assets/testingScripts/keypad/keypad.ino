#include <Servo.h>
#include <Keypad.h>

#define SERVO 10 // wings servos
#define FRONT 90
Servo head;
int value;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {1, 2, 3, 4};
byte colPins[COLS] = {5, 11, 12, 13};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void setup() {
  // put your setup code here, to run once:
  head.attach(SERVO);
  head.write(FRONT);  // Start with the servo in the FRONT position
  Serial.begin(9600); // Add Serial.begin() to print the key press
}

void loop() {
  // put your main code here, to run repeatedly:
  char customKey = customKeypad.getKey();

  if (customKey) {
    Serial.println(customKey); // Print the key pressed for debugging
  }

  if (customKey == '1') {
    unsigned long currentTime = millis();

    // Check if enough time has passed to avoid bouncing
    if (currentTime - lastDebounceTime > debounceDelay) {
      head.write(FRONT + 33);  // Move the servo to the new position
      lastDebounceTime = currentTime;  // Update the debounce time

      // Optionally, move the servo back to FRONT after a short delay
      delay(500);  // Wait for 500ms before moving the servo back
      head.write(FRONT);  // Move the servo back to the FRONT position
    }
  }
}
