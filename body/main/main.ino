#include <WiFi.h>
#include <HTTPClient.h>

#define PIR_PIN 15  // Or your connected GPIO pin

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

const char* api_url = "http://your-api.com/motion";  // Replace with your API endpoint

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.println(WiFi.localIP());
}

void loop() {
  int motion = digitalRead(PIR_PIN);

  // Only send when motion is detected
  if (motion == HIGH) {
    sendMotionData(true);
    delay(5000);  // Wait to prevent spamming the server
  }

  delay(200);
}

void sendMotionData(bool motionDetected) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(api_url);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"motion\": " + String(motionDetected ? "true" : "false") + "}";
    int httpResponseCode = http.POST(payload);

    Serial.print("Sent motion: ");
    Serial.println(payload);
    Serial.print("Response code: ");
    Serial.println(httpResponseCode);

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}
