#pragma once

#include "constants.hpp"
#include <Adafruit_LIS3DH.h>


Adafruit_LIS3DH lis3dh = Adafruit_LIS3DH();


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