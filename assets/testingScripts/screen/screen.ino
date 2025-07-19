#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("ST7796S Test");
}

void loop() {
  tft.fillRect(50, 50, 100, 100, TFT_RED);
  delay(500);
  tft.fillRect(50, 50, 100, 100, TFT_BLUE);
  delay(500);
}
