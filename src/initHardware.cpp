#include "constants.hpp"

void initHardware() {
  System.enableReset(); //allows System.reset() to work
  pinMode(kBLEConnectedLED, OUTPUT); //BLE connected indicator 
  digitalWrite(kBLEConnectedLED, LOW);
  if(!lis3dh.begin(kLis3dhAddress)) {
    delay(1000);
    WITH_LOCK(Serial) {
      Serial.println("Failed to initialize LIS3DH");
    }
  }
}