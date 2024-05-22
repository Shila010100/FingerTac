// This is the code for the FingerTac with the I2C Multiplexer TCA9548A
// Board Manager: https://dl.espressif.com/dl/package_esp32_index.json
// Board: 'ESP32 Arduino' -> M5Stick-C
//Set in Serial Monitor: 115200 baud

#include <WiFi.h>
#include <WebSocketsServer.h>
#include <M5StickCPlus.h>
#include "Adafruit_DRV2605.h"
#include "TCA9548A.h"
#include <esp_now.h>

// WiFi credentials
const char* ssid = "FRITZ!Box 7530 KJ"; // Alex home network: "FRITZ!Box 7530KJ"
const char* password = "38874388762792916547"; // Alex home network pw: "38874388762792916547"

// WebSocket server
WebSocketsServer webSocket = WebSocketsServer(81); // Port 81 for WebSocket

// include images:
#include "include/draw_ceti_logo.h"
//#include "include/hand_bw_10.h"

// declare function prototype:
// void simulateWiFiCommand(String command); // this is only to simulate the wifi connection

// Structure of espnow-message - must match the structure of the other participants
struct espnow_haptic_message_struct {
  int message_counter;
  uint8_t receiver_device_id;  // // 0:left; 1:right; 2:controller;
  uint8_t intensities[3];
};
espnow_haptic_message_struct haptic_message;

Adafruit_DRV2605 drv[3];
//TCA9548A I2CMux_TCA9548A;                  // Address can be passed into the constructor
#define I2CMUX_TCA9548A_ADDR 0x70

bool g_isRightHandOrientation=true;
byte g_screen_orientation=1;

static const char PWM_PIN = 25;  // 32 on the old M5StickC, 25 on the newer M5StickCPlus
char PIN_1 = 1;
int pwmMinVal = 0;
int pwmMaxVal = 255;
//byte DRV2605_ADDR = 0x5A;     //DRV2605 slave address
//byte DRV2605_REG_MODE = 0x01; //I2C address for Mode Register on DRV2605
byte effect_tick_val = 23;
byte effect_soft_val = 3;
byte effect_strong_val = 120;

uint8_t FingerTac_operation_mode = 1;
// LED parameters:
//const unsigned char LED_INTENSITY = 75;  // High intensity probably drains the battery faster, low intensity is less visible. Feel free to play around with this parameter, the range is 0 to 255
const int M5STICKCPLUS_LED_PIN = 10;  // = LED_BUILTIN
int counter_last_vibration_if_no_tof_measured = 0;

int screen_brightness = 10;  // [7 .. 12]
bool is_drv_rt_mode = false;
int g_display_counter = 0;

void vibrateMotors(uint8_t intensities[3]) {
  Serial.print("Vibrating motors with intensities: "); // added 5 lines.. serial print for debugging the connection with unity
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

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  espnow_haptic_message_struct haptic_message_last = haptic_message;
  memcpy(&haptic_message, incomingData, sizeof(haptic_message));
  vibrateMotors(haptic_message.intensities);
  udpateVisuBars(haptic_message.intensities,g_screen_orientation);
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
    M5.Lcd.setRotation(g_screen_orientation); // Turn the screen sidewaysc:\Users\Melanie\Downloads\dlr\FingerTac\git\FingerTac\2024_02_for_zachmann\FingerTacZachmann\include\draw_ceti_logo.h
  }
  return g_screen_orientation;
}

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connection from ", num);
            Serial.println(ip.toString());
            break;
        }
        case WStype_TEXT:
            Serial.printf("[%u] Text: %s\n", num, payload);
            // Assume payload is in the format "255,0,0"
            uint8_t intensities[3] = {0, 0, 0};
            sscanf((const char*)payload, "%hhu,%hhu,%hhu", &intensities[0], &intensities[1], &intensities[2]);
            vibrateMotors(intensities);
            break;
        case WStype_BIN:
            Serial.printf("[%u] Binary: %u bytes\n", num, length);
            break;
    }
}

void setup() {
    M5.begin();
    Serial.println("FingerTac v1.2");
    setScreenBrightness(screen_brightness);
    g_screen_orientation = updateScreenOrientation(g_isRightHandOrientation);
    drawCetiBackground(g_screen_orientation);
    M5.Lcd.setCursor(13, 110);
    M5.Lcd.print("DLR FingerTac V1.2");

    Serial.println(" - activating LED");
    pinMode(M5STICKCPLUS_LED_PIN, OUTPUT);
    Serial.print(" - initializing haptic drivers:");

    Wire.begin(32, 33);
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
        digitalWrite(M5STICKCPLUS_LED_PIN, HIGH);
        delay(200);
        digitalWrite(M5STICKCPLUS_LED_PIN, LOW);
        tca9548a_select(i);
        drv[i].setWaveform(0, 1);
        drv[i].setWaveform(1, 0);
        drv[i].go();
        delay(300);
    }
    Serial.println("");
    digitalWrite(M5STICKCPLUS_LED_PIN, HIGH);
    delay(500);
    drawCetiSignetBackground(false, g_screen_orientation);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    Serial.println("setup() finished");
    Serial.println("entering loop");
}

void loop() {
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
        digitalWrite(M5STICKCPLUS_LED_PIN, LOW);
        if (is_drv_rt_mode) {
            drv[0].setRealtimeValue(0x0);
            drv[0].setMode(DRV2605_MODE_INTTRIG);
            is_drv_rt_mode = false;
        }
        drv[0].setWaveform(0, effect_soft_val);
        drv[0].setWaveform(1, 0);
        drv[0].go();
        delay(200);
        digitalWrite(M5STICKCPLUS_LED_PIN, HIGH);
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
    M5.update();

    webSocket.loop();
}
