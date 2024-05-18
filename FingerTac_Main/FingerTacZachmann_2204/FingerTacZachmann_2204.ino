#include <WiFi.h>
#include <M5StickCPlus.h>
#include "Adafruit_DRV2605.h"
#include "TCA9548A.h"
#include <esp_now.h>

// Include images
#include "include/draw_ceti_logo.h"
//#include "include/hand_bw_10.h"

struct espnow_haptic_message_struct {
  int message_counter;
  uint8_t receiver_device_id;  // 0: left; 1: right; 2: controller;
  uint8_t intensities[3];
};
espnow_haptic_message_struct haptic_message;

Adafruit_DRV2605 drv[3];
#define I2CMUX_TCA9548A_ADDR 0x70

bool g_isRightHandOrientation = true;
byte g_screen_orientation = 1;

const int M5STICKCPLUS_LED_PIN = 10;  // = LED_BUILTIN

int screen_brightness = 10;  // Range: 7 to 12
bool is_drv_rt_mode = false;
int g_display_counter = 0;

void setup() {
  M5.begin();
  Serial.begin(115200); // Initialize serial communication
  setScreenBrightness(screen_brightness);
  g_screen_orientation = updateScreenOrientation(g_isRightHandOrientation);
  drawCetiBackground(g_screen_orientation);
  
  pinMode(M5STICKCPLUS_LED_PIN, OUTPUT);
  Wire.begin(32, 33);  // Start I2C on specific pins if necessary

  for (uint8_t i = 0; i < 3; i++) {
    tca9548a_select(i);
    drv[i].begin();
    drv[i].selectLibrary(1); // Assuming use of ERM motors
    drv[i].setMode(DRV2605_MODE_INTTRIG);
  }

  // Optional: Initialize ESP-NOW here if used in your project
  // ESP-NOW Initialization Code
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Setup completed");
}
void loop() {
  // Example of handling button presses to adjust screen brightness or change operation modes
  // Button handling code here

  // Serial command handling for vibration patterns
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command.startsWith("vibrate 48")) { // changed from "vibrate ?" to "vibrate 48"
      // Assuming format "vibrate X", where X is the effect number
      for (uint8_t i = 0; i < 3; i++) {
        triggerVibrationEffect(i, 48); //changed from ? vibrate to vibrate 48
      }
    }
  }

  delay(100); // Small delay to avoid overwhelming the CPU
}

void vibrateMotors(uint8_t intensities[3]) {
  for (uint8_t i = 0; i < 3; i++) {
    tca9548a_select(i);
    drv[i].setWaveform(0, intensities[i]); // Assuming intensity values map directly to effect numbers
    drv[i].setWaveform(1, 0); // End waveform
    drv[i].go();
  }
}

void triggerVibrationEffect(uint8_t motorIndex, uint8_t effect) {
  if (motorIndex < 3) {
    tca9548a_select(motorIndex);
    drv[motorIndex].setMode(DRV2605_MODE_INTTRIG);
    drv[motorIndex].setWaveform(0, effect);
    drv[motorIndex].setWaveform(1, 0); // End waveform sequence
    drv[motorIndex].go();
  }
}
uint8_t g_intensities_last[3] = {0, 0, 0};
void udpateVisuBars(uint8_t intensities[3], byte screen_orientation) {
  if (g_display_counter++ >= 0) {
    int xPos = 0;
    int yPos = 0;
    if (screen_orientation == 1) {
      xPos = 186;
    } else {
      xPos = 0;
    }
    // M5StickC PLUS display: 135 * 240
    //M5.Lcd.fillRect(184, 15, 48, 105, 0x07FF); // done in setup
    for (uint8_t i = 0; i < 3; i++) {
      if (screen_orientation == 1) {
        yPos = 32 + (2-i) * 30;
      } else {
        yPos = 32 + i * 30;
      }
      if (g_intensities_last[i] != intensities[i]) {
        if (intensities[i] < g_intensities_last[i]) {
          M5.Lcd.fillRect(xPos + (int)(44. * (intensities[i] / 255.)), yPos, 1 + (int)(44. * (1.0 - (intensities[i] / 255.))), 16, 0x07FF);
        }
        if (intensities[i] > 20) {
          M5.Lcd.fillRect(xPos, yPos, (int)(44. * (intensities[i] / 255.)), 16, BLUE);
        }
      }
      g_intensities_last[i] = intensities[i];
    }
    g_display_counter = 0;
  }
}
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  espnow_haptic_message_struct haptic_message_last = haptic_message;
  memcpy(&haptic_message, incomingData, sizeof(haptic_message));
  vibrateMotors(haptic_message.intensities);
  udpateVisuBars(haptic_message.intensities,g_screen_orientation);
}

uint8_t tca9548a_select(uint8_t channel) {
  if (channel > 7) return 255; // Channel out of bounds
  Wire.beginTransmission(I2CMUX_TCA9548A_ADDR);
  Wire.write(1 << channel);
  return Wire.endTransmission();
}

void setScreenBrightness(int brightness) {
  // Adjust the screen brightness, specific to your hardware setup
  M5.Axp.ScreenBreath(brightness);
}

byte updateScreenOrientation(bool isRightHandOrientation) {
  byte orientation = isRightHandOrientation ? 3 : 1;
  M5.Lcd.setRotation(orientation);
  return orientation;
}

void drawCetiBackground(bool show_esp_now_logo=true, byte screen_orientation=1) {
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


}
// Additional functions like drawCetiSignetBackground() as needed
