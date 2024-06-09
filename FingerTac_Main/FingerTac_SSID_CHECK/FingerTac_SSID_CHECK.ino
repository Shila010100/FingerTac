#include <WiFi.h>
#include <M5StickCPlus.h>

void setup() {
  M5.begin();
  Serial.begin(115200);

  WiFi.mode(WIFI_OFF);  // Turn off WiFi to reset any previous state
  delay(1000);  // Ensure the WiFi is completely turned off

  WiFi.mode(WIFI_AP);
  bool apSuccess = WiFi.softAP("FingerTac_Pink", "12345678");  // Use simple SSID and password

  Serial.println("AP Mode activated:");
  Serial.print("Current SSID: ");
  Serial.println(WiFi.softAPSSID());

  if (apSuccess) {
    Serial.println("AP started successfully");
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start AP");
  }
}

void loop() {
  // Empty loop
}
