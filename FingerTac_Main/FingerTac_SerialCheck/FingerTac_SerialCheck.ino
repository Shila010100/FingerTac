void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for the serial port to connect - necessary for Leonardo/Micro only
  Serial.println("Hello, world!");
}

void loop() {
  // Optional: Echo any received characters
  if (Serial.available() > 0) {
    int incomingByte = Serial.read();
    Serial.print("Received: ");
    Serial.println(char(incomingByte));
  }
  delay(1000);
}
