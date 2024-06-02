// include images:
#include "ceti_logo_50.h"
#include "ceti_signet.h"
#include "esp_now_152x38.h"

void drawCetiBackground(byte screen_orientation=1) {
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setTextSize(2);
  //M5.Lcd.setRotation(screen_orientation); // Turn the screen sideways
  if (screen_orientation == 1) {
    M5.Lcd.pushImage(48, 32, ceti_logo_imgWidth, ceti_logo_imgHeight, ceti_logo_img); // https://lang-ship.com/tools/image2data/
  } else {
    M5.Lcd.pushImage(48, 32, ceti_logo_imgWidth, ceti_logo_imgHeight, ceti_logo_img); // https://lang-ship.com/tools/image2data/
  }
  //M5.Lcd.setSwapBytes(false); // Change colours if they are wrong
  //M5.Lcd.setCursor(13, 110);
  //M5.Lcd.println("DLR FingerTac V1.0");
}

void drawCetiSignetBackground(bool show_esp_now_logo=true, byte screen_orientation=1) {
  M5.Lcd.fillScreen(WHITE);
  //M5.Lcd.pushImage(160, 29, ceti_signet_imgWidth, ceti_signet_imgHeight, ceti_signet_img);
  //M5.Lcd.pushImage(30, 52, esp_now_imgWidth, esp_now_imgHeight, esp_now_img);
  // M5StickC PLUS display: 135 * 240
  //M5.Lcd.pushImage(120 - 34, 4, ceti_signet_imgWidth, ceti_signet_imgHeight, ceti_signet_img);
if (screen_orientation == 1) {
  if (show_esp_now_logo) {
    M5.Lcd.pushImage(48 - 29, 10, ceti_logo_imgWidth, ceti_logo_imgHeight, ceti_logo_img);
    M5.Lcd.pushImage(120 - 76 - 29, 88, esp_now_imgWidth, esp_now_imgHeight, esp_now_img);
  } else {
    M5.Lcd.pushImage(48 - 29, 40, ceti_logo_imgWidth, ceti_logo_imgHeight, ceti_logo_img);
  }
  M5.Lcd.fillRect(184, 15, 48, 105, 0x07FF);
} else {
  if (show_esp_now_logo) {
    M5.Lcd.pushImage(50+48 - 29, 10, ceti_logo_imgWidth, ceti_logo_imgHeight, ceti_logo_img);
    M5.Lcd.pushImage(50+120 - 76 - 29, 88, esp_now_imgWidth, esp_now_imgHeight, esp_now_img);
  } else {
    M5.Lcd.pushImage(50+48 - 29, 40, ceti_logo_imgWidth, ceti_logo_imgHeight, ceti_logo_img);
  }
  M5.Lcd.fillRect(0, 15, 48, 105, 0x07FF);
}
//  old_hand_pos = -1.0; // causes hand icon being redrawn
}
