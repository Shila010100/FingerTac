#include <WiFi.h>
#include <M5StickCPlus.h>
#include "Adafruit_DRV2605.h"
#include "TCA9548A.h"
#include <WebSocketsServer.h>

#include "include/draw_ceti_logo.h"

// AP credentials
const char* ssid = "FingerTac_Pink";
const char* password = "Shila";  // Set a password for the AP

// WebSocket server
WebSocketsServer webSocket = WebSocketsServer(80);

Adafruit_DRV2605 drv[3];
#define I2CMUX_TCA9548A_ADDR 0x70

void vibrateMotors(uint8_t intensities[3]) {
  unsigned long startTime = millis();
  for (uint8_t i = 0; i < 3; i++) {
    if (intensities[i] > 20) {
      tca9548a_select(i);
      drv[i].setWaveform(0, 1);
      drv[i].setWaveform(1, 0);
      drv[i].go();
    }
  }
  Serial.print("Vibration command processed in ");
  Serial.print(millis() - startTime);
  Serial.println(" ms");
}

uint8_t tca9548a_select(uint8_t channel) {
  if (channel > 7) return 255;
  Wire.beginTransmission(I2CMUX_TCA9548A_ADDR);
  Wire.write(1 << channel);
  return Wire.endTransmission();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_BIN && length == 3) {
    Serial.printf("[%u] Received binary data, processing vibration...\n", num);
    vibrateMotors(payload);
  }
}

void setup() {
  M5.begin();
  Serial.println("Initializing FingerTac Device in AP Mode");

  // Set brightness to lowest but visible setting
  M5.Axp.ScreenBreath(7);
  M5.Lcd.setRotation(3);  // Orientation set based on physical placement
  drawCetiBackground();  // Make sure to include your graphics correctly

  // Initialize WiFi in AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println("AP Mode activated");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Start WebSocket server on AP
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started on AP");

  // Initialize haptic drivers
  Wire.begin(32, 33);
  for (uint8_t i = 0; i < 3; i++) {
    tca9548a_select(i);
    drv[i].begin(&Wire);
    drv[i].selectLibrary(1);
    drv[i].setMode(DRV2605_MODE_INTTRIG);
    drv[i].useLRA();
  }
}

void loop() {
  webSocket.loop();
}
