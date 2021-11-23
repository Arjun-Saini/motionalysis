#pragma once
#include "globalVariables.hpp"
#include "constants.hpp"
#include <Adafruit_LIS3DH.h>

Adafruit_LIS3DH lis3dh = Adafruit_LIS3DH();

void writeRegister(byte reg, byte data) {
  Wire.beginTransmission(kLis3dhAddress);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

void init_ACC(void) {
//   writeRegister(0x20, 0xA7); //Write A7h into CTRL_REG1;      // Turn on the sensor with ODR = 100Hz normal mode.
//   writeRegister(0x21, 0x00); //Write 00h into CTRL_REG2;      // High-pass filter (HPF) disabled 
//   writeRegister(0x22, 0x40); //Write 40h into CTRL_REG3;      // ACC AOI1 interrupt signal is routed to INT1 pin.
// //  writeRegister(0x23, 0x00); //Write 00h into CTRL_REG4;      // Full Scale = +/-2 g
  writeRegister(0x24, 0x00); //Write 00h into CTRL_REG5;      // Default value. Interrupt signals on INT1 pin is not latched. 
  writeRegister(0x32, 0x10); //Write 10h into INT1_THS;          // Threshold (THS) = 16LSBs * 15.625mg/LSB = 250mg.
  writeRegister(0x33, 0x00); //Write 33h into INT1_DURATION;     // Duration = 1LSBs * (1/10Hz) = 0.1s.
  writeRegister(0x30, 0x0A); //Write 0Ah into INT1_CFG;          // Enable XLIE, YLIE interrupt generation, OR logic.
}


void initHardware() {
  System.enableReset(); //allows System.reset() to work
  pinMode(kBLEConnectedLED, OUTPUT); //BLE connected indicator 
  digitalWrite(kBLEConnectedLED, LOW);
  pinMode(kLIS3DHInterruptPin, INPUT); //LIS3DH interrupt pin
  sleepConfig.mode(SystemSleepMode::HIBERNATE)
    .gpio(kLIS3DHInterruptPin, RISING); //lowest power that does not resume as if from a reset
  
  while(!Serial.isConnected()){}
  if(!lis3dh.begin(kLis3dhAddress)) {
    delay(1000);
    WITH_LOCK(Serial) {
      Serial.println("Failed to initialize LIS3DH");
    }
  }
  init_ACC();
}

void initFromEEPROM() {
  EEPROM.get(kRecordingIntervalEEPROMAddress, recordingInterval);
  EEPROM.get(kDsidEEPROMAddress, dsid);
  EEPROM.get(kReportingIntervalEEPROMAddress, reportingInterval);
  EEPROM.get(kSleepPauseDurationEEPROMAddress, sleepPauseDuration);
  reportingInterval = reportingInterval / 1000; // convert to seconds from milliseconds 
  Serial.printlnf("recordingInterval: %i", recordingInterval);
  Serial.printlnf("reportingInterval: %i", reportingInterval);
  if(recordingInterval == kEEPROMEmptyValue) { // if no value stored in EEPROM, set to default
    recordingInterval = kDefaultRecordingInterval; //default value
  }
  if(reportingInterval == kEEPROMEmptyValue) {
    reportingInterval = kDefaultReportingInterval; //default value
  }
  if(sleepPauseDuration == kEEPROMEmptyValue) {
    sleepPauseDuration = kDefaultSleepPauseDuration; //default value
  }
  if(dsid == kEEPROMEmptyValue) {
    Serial.println("DSID not stored in EEPROM. BLE config required"); 
    //TODO notify user somehow
  }
}