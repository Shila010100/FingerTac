#include <WiFi.h>
#include <M5StickCPlus.h>
#include "Adafruit_DRV2605.h"
#include "TCA9548A.h"

// WiFi credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// WiFi server
WiFiServer server(8080);

// Haptic driver instances
Adafruit_DRV2605 drv[3];
#define I2CMUX_TCA9548A_ADDR 0x70

bool g_isRightHandOrientation = true;
byte g_screen_orientation = 1;

void vibrateMotors(uint8_t intensities[3]) {
  for (uint8_t i = 0; i < 3; i++) {
    if (intensities[i] > 20) {
      tca9548a_select(i);
      drv[i].setWaveform(0, 1);  // effect 1
      drv[i].setWaveform(1, 0);  // end waveform
      drv[i].go();
    }
  }
}

uint8_t tca9548a_select(uint8_t channel) {
  if (channel > 7) return 255;
  Wire.beginTransmission(I2CMUX_TCA9548A_ADDR);
  Wire.write(1 << channel);
  return Wire.endTransmission();
}

void setup() {
  M5.begin();
  Serial.begin(115200);
  setScreenBrightness(screen_brightness);
  g_screen_orientation = updateScreenOrientation(g_isRightHandOrientation);
  drawCetiBackground(g_screen_orientation);
  M5.Lcd.setCursor(13, 110);
  M5.Lcd.print("DLR FingerTac V1.2");

  pinMode(M5STICKCPLUS_LED_PIN, OUTPUT);

  Wire.begin(32, 33);  // start I2C on pins 32 (SDA) and 33 (SCL)
  for (uint8_t i = 0; i < 3; i++) {
    tca9548a_select(i);
    drv[i].begin(&Wire);
    drv[i].selectLibrary(1);
    drv[i].setMode(DRV2605_MODE_INTTRIG);
    drv[i].useLRA();
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("Server started.");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected.");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n' && currentLine.length() == 0) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.println("<!DOCTYPE html><html>");
          client.println("<body><h1>FingerTac</h1></body></html>");
          client.println();
          break;
        } else if (c != '\r') {
          currentLine += c;
        }
        if (c == '\n') {
          Serial.println(currentLine);
          if (currentLine.startsWith("GET /vibrate?intensities=")) {
            int index = currentLine.indexOf('=') + 1;
            String values = currentLine.substring(index);
            uint8_t intensities[3];
            int idx = 0;
            for (int i = 0; i < values.length(); i++) {
              if (values[i] == ',' || i == values.length() - 1) {
                if (i == values.length() - 1) i++;
                intensities[idx++] = values.substring(0, i).toInt();
                values = values.substring(i + 1);
                i = -1;
              }
            }
            vibrateMotors(intensities);
          }
          currentLine = "";
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected.");
  }
}

void setScreenBrightness(int screen_brightness) {
  if (screen_brightness <= 7) {
    M5.Axp.ScreenBreath(0);
  } else {
    M5.Axp.ScreenBreath((screen_brightness - 7) * 15);
  }
}

byte updateScreenOrientation(bool isRightHandOrientation) {
  if (isRightHandOrientation) {
    g_screen_orientation = 3;
    M5.Lcd.setRotation(g_screen_orientation);
  } else {
    g_screen_orientation = 1;
    M5.Lcd.setRotation(g_screen_orientation);
  }
  return g_screen_orientation;
}