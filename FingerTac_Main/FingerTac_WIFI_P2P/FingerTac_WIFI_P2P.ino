// This is the code for the FingerTac with the I2C Multiplexer TCA9548A
// Board Manager: https://dl.espressif.com/dl/package_esp32_index.json
// Board: 'ESP32 Arduino' -> M5Stick-C
// Set in Serial Monitor: 115200 baud

#include <WiFi.h>
#include <M5StickCPlus.h>
#include "Adafruit_DRV2605.h"
#include "TCA9548A.h"
#include <esp_now.h>

// WiFi server
WiFiServer server(8080);

// include images:
#include "include/draw_ceti_logo.h"
//#include "include/hand_bw_10.h"

// Structure of espnow-message - must match the structure of the other participants
struct espnow_haptic_message_struct {
  int message_counter;
  uint8_t receiver_device_id;  // // 0:left; 1:right; 2:controller;
  uint8_t intensities[3];
};
espnow_haptic_message_struct haptic_message;

Adafruit_DRV2605 drv[3];
#define I2CMUX_TCA9548A_ADDR 0x70

bool g_isRightHandOrientation = true;
byte g_screen_orientation = 1;

static const char PWM_PIN = 25;  // 32 on the old M5StickC, 25 on the newer M5StickCPlus
char PIN_1 = 1;
int pwmMinVal = 0;
int pwmMaxVal = 255;
byte effect_tick_val = 23;
byte effect_soft_val = 3;
byte effect_strong_val = 120;

uint8_t FingerTac_operation_mode = 1;
const int M5STICKCPLUS_LED_PIN = 10;  // = LED_BUILTIN
int counter_last_vibration_if_no_tof_measured = 0;

int screen_brightness = 10;  // [7 .. 12]
bool is_drv_rt_mode = false;
int g_display_counter = 0;

void vibrateMotors(uint8_t intensities[3]) {
  Serial.print("Vibrating motors with intensities: ");
  for (int i = 0; i < 3; i++) {
    Serial.print(intensities[i]);
    if (i < 2) Serial.print(", ");
  }
  Serial.println();

  for (uint8_t i = 0; i < 3; i++) {
    if (intensities[i] > 20) {
      tca9548a_select(i);
      drv[i].setWaveform(0, 1);  // effect 1
      drv[i].setWaveform(1, 0);  // end waveform
      drv[i].go();
      counter_last_vibration_if_no_tof_measured = 0;
    }
  }
}

void vibrateMotorsWaveform(uint8_t waveforms[3]) {
  for (uint8_t i = 0; i < 3; i++) {
    if (waveforms[i] > 0) {
      tca9548a_select(i);
      drv[i].setWaveform(0, waveforms[i]);  // effect no see https://www.ti.com/lit/ds/symlink/drv2605.pdf#page=57
      drv[i].setWaveform(1, 0);  // end waveform
      drv[i].go();
      counter_last_vibration_if_no_tof_measured = 0;
    }
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
    for (uint8_t i = 0; i < 3; i++) {
      if (screen_orientation == 1) {
        yPos = 32 + (2 - i) * 30;
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

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  espnow_haptic_message_struct haptic_message_last = haptic_message;
  memcpy(&haptic_message, incomingData, sizeof(haptic_message));
  vibrateMotors(haptic_message.intensities);
  udpateVisuBars(haptic_message.intensities, g_screen_orientation);
}

uint8_t tca9548a_select(uint8_t channel) {
  if (channel > 7) return 255;
  Wire.beginTransmission(I2CMUX_TCA9548A_ADDR);
  Wire.write(1 << channel);
  return Wire.endTransmission();
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
    M5.Lcd.setRotation(g_screen_orientation); // Turn the screen sideways
  } else {
    g_screen_orientation = 1;
    M5.Lcd.setRotation(g_screen_orientation); // Turn the screen sideways
  }
  return g_screen_orientation;
}

void setup() {
  M5.begin();
  Serial.begin(115200);
  Serial.println("FingerTac v1.2");
  setScreenBrightness(screen_brightness);
  g_screen_orientation = updateScreenOrientation(g_isRightHandOrientation);
  drawCetiBackground(g_screen_orientation);
  M5.Lcd.setCursor(13, 110);
  M5.Lcd.print("DLR FingerTac V1.2");
  Serial.println(" - activating LED");
  pinMode(M5STICKCPLUS_LED_PIN, OUTPUT);
  Serial.print(" - initializing haptic drivers:");
  Wire.begin(32, 33);  // start I2C on pins 32 (SDA) and 33 (SCL) -> on grove-plug of M5Stick
  for (uint8_t i = 0; i < 3; i++) {
    Serial.print(" ");
    Serial.print(i);
    tca9548a_select(i);
    drv[i].begin(&Wire);
    drv[i].selectLibrary(1);
    drv[i].setMode(DRV2605_MODE_INTTRIG);
    is_drv_rt_mode = false;
    drv[i].useLRA();
  }
  Serial.println("");
  Serial.print("- vibration test for motor:");
  for (uint8_t i = 0; i < 3; i++) {
    Serial.print(" ");
    Serial.print(i);
    digitalWrite(M5STICKCPLUS_LED_PIN, HIGH);  // HIGH = led off
    delay(200);
    digitalWrite(M5STICKCPLUS_LED_PIN, LOW);  // LOW = led on
    tca9548a_select(i);
    drv[i].setWaveform(0, 1);  // play effect
    drv[i].setWaveform(1, 0);  // end waveform
    drv[i].go();
    delay(300);
  }
  Serial.println("");
  digitalWrite(M5STICKCPLUS_LED_PIN, HIGH);  // HIGH = led off
  delay(500);
  drawCetiSignetBackground(false, g_screen_orientation);

  // Init ESP-NOW
  WiFi.mode(WIFI_STA);  // Set device as a Wi-Fi Station
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("setup() finished");
  Serial.println("entering loop");

  // Setup WiFi Direct (P2P)
  Serial.print("Setting up WiFi Direct...");
  WiFi.mode(WIFI_AP);
  if (WiFi.softAP("M5StickCPlus_P2P")) {
    Serial.println("WiFi Direct setup complete.");
    Serial.println("IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("WiFi Direct setup failed.");
  }

  // Start the server
  server.begin();
  Serial.println("Server started.");
}

void loop() {
  uint8_t i = 0;

  M5.BtnB.read();
  if (M5.BtnB.wasPressed()) {
    screen_brightness = screen_brightness + 2;
    if (screen_brightness > 12) {
      screen_brightness = 6;
    }
    setScreenBrightness(screen_brightness);
  }
  M5.BtnA.read();
  if (M5.BtnA.wasPressed()) {
    digitalWrite(M5STICKCPLUS_LED_PIN, LOW);  // LOW = led on
    if (is_drv_rt_mode) {
      drv[i].setRealtimeValue(0x0);
      drv[i].setMode(DRV2605_MODE_INTTRIG);
      is_drv_rt_mode = false;
    }
    drv[i].setWaveform(0, effect_soft_val);  // play effect: effect_soft_val, effect_strong_val
    drv[i].setWaveform(1, 0);                // end waveform
    drv[i].go();

    delay(200);
    digitalWrite(M5STICKCPLUS_LED_PIN, HIGH);  // HIGH = led off
    FingerTac_operation_mode = FingerTac_operation_mode + 1;
    if (FingerTac_operation_mode >= 5) {
      FingerTac_operation_mode = 0;
    }
    Serial.print("Mode set to: ");
    switch (FingerTac_operation_mode) {
      case 0: Serial.println("(0) welcome screen"); break;
      case 1: Serial.println("(1) serial monitor"); break;
      case 2: Serial.println("(2) espnow"); break;
      case 3: Serial.println("(3) debug"); break;
      case 4: Serial.println("(4) vibration test"); break;
    }
    if (screen_brightness <= 7) {
      screen_brightness = 8;
      setScreenBrightness(screen_brightness);
    }
    if (FingerTac_operation_mode == 0) {
      setScreenBrightness(screen_brightness);
      g_isRightHandOrientation = !g_isRightHandOrientation;
      if (g_isRightHandOrientation) {
        Serial.println("orientation changed to right hand");
      } else {
        Serial.println("orientation changed to left hand");
      }

      g_screen_orientation = updateScreenOrientation(g_isRightHandOrientation);
      drawCetiBackground(g_screen_orientation);
      M5.Lcd.setCursor(13, 110);
      M5.Lcd.print("DLR FingerTac V1.2");
    }
    if (FingerTac_operation_mode == 1) {
      drawCetiSignetBackground(false, g_screen_orientation);
    }
    if (FingerTac_operation_mode == 2) {
      drawCetiSignetBackground(true, g_screen_orientation);
    }
    if (FingerTac_operation_mode == 3) M5.Lcd.fillScreen(BLUE);
  }
  M5.update();  // required for buttons
  if (FingerTac_operation_mode == 1) {
    uint8_t NUM_INTENSITIES = 3;
    uint8_t intensities[NUM_INTENSITIES];
    uint8_t waveforms[NUM_INTENSITIES];
    for (int i = 0; i < NUM_INTENSITIES; i++) {
      intensities[i] = 0;
    }
    if (Serial.available() >= 6) {
      for (int i = 0; i < NUM_INTENSITIES; i++) {
        waveforms[i] = Serial.parseInt();
        if (waveforms[i] > 0) intensities[i] = 255; else intensities[i] = 0;
      }
      vibrateMotorsWaveform(waveforms);
      udpateVisuBars(intensities, g_screen_orientation);
      Serial.print("Received Waveform: ");
      for (int i = 0; i < NUM_INTENSITIES; i++) {
        Serial.print(waveforms[i]);
        Serial.print(" ");
      }
      Serial.println();
    } else {
      udpateVisuBars(intensities, g_screen_orientation);
    }

    delay(200);
  } else if (FingerTac_operation_mode == 2) {
    delay(200);
  } else if (FingerTac_operation_mode == 3) {
    counter_last_vibration_if_no_tof_measured++;
    M5.Lcd.setTextColor(WHITE, BLUE);
    M5.Lcd.setCursor(45, 2);
    M5.Lcd.print("FingerTac Debug");
    M5.Lcd.setCursor(4, 26);
    M5.Lcd.print("battery:  ");
    M5.Lcd.print(M5.Axp.GetBatVoltage());
    M5.Lcd.print("   ");
    M5.Lcd.setCursor(4, 44);
    M5.Lcd.setCursor(4, 116);
    M5.Lcd.setTextSize(1);
    M5.Lcd.print("mac: ");
    M5.Lcd.print(WiFi.macAddress());
    M5.Lcd.setTextSize(2);
  } else if (FingerTac_operation_mode == 4) {
    for (uint8_t i = 0; i < 3; i++) {
      digitalWrite(M5STICKCPLUS_LED_PIN, HIGH);  // HIGH = led off
      delay(200);
      digitalWrite(M5STICKCPLUS_LED_PIN, LOW);  // LOW = led on
      tca9548a_select(i);
      drv[i].setWaveform(0, 1);  // play effect
      drv[i].setWaveform(1, 0);  // end waveform
      drv[i].go();
    }
    delay(400);
  }
  delay(100);

  // Handle incoming clients
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected.");
    String currentLine = "";
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c); // Debug print for received characters
        if (c == '\n' && currentLineIsBlank) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.println("<!DOCTYPE html><html>");
          client.println("<body><h1>FingerTac</h1></body></html>");
          client.println();
          break;
        } else if (c == '\n') {
          if (currentLine.length() > 0) {
            Serial.println("Received request: " + currentLine); // Add this line
            if (currentLine.startsWith("GET /vibrate?intensities=")) {
              Serial.println("Parsing intensities..."); // Debug print for parsing start
              int index = currentLine.indexOf('=') + 1;
              String values = currentLine.substring(index);
              uint8_t intensities[3] = {0, 0, 0};
              int idx = 0;
              for (int i = 0; i < values.length(); i++) {
                if (values[i] == ',' || i == values.length() - 1) {
                  if (i == values.length() - 1) i++;
                  intensities[idx++] = values.substring(0, i).toInt();
                  values = values.substring(i + 1);
                  i = -1;
                }
              }
              Serial.print("Intensities parsed: ");
              for (int i = 0; i < 3; i++) {
                Serial.print(intensities[i]);
                if (i < 2) Serial.print(", ");
              }
              Serial.println();
              vibrateMotors(intensities);
            }
            currentLine = "";
          }
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLine += c;
          currentLineIsBlank = false;
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected.");
  }
}
