#pragma once
#include "globalVariables.hpp"
#include "constants.hpp"
#include <Adafruit_LIS3DH.h>
#include "WatchDog_WCL.h"

Adafruit_LIS3DH lis3dh = Adafruit_LIS3DH();
WatchDog wd = WatchDog();

void writeRegister(byte reg, byte data) {
  Wire.beginTransmission(kLis3dhAddress);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

unsigned int readRegister(byte reg) {
  Wire.beginTransmission(kLis3dhAddress);
  Wire.write(reg);
  Wire.endTransmission();
 
  Wire.requestFrom(kLis3dhAddress, 1);
  return Wire.read();
}

void init_ACC(void) {
  readRegister(0x21);
  readRegister(0x26);
  readRegister(LIS3DH_REG_INT1SRC);
  writeRegister(0x20, 0x57); //Write A7h into CTRL_REG1;      // Turn on the sensor, enable X, Y, Z axes with ODR = 100Hz normal mode.
  writeRegister(0x21, 0x09); //Write 09h into CTRL_REG2;      // High-pass filter (HPF) enabled
  writeRegister(0x22, 0x40); //Write 40h into CTRL_REG3;      // ACC AOI1 interrupt signal is routed to INT1 pin.
  writeRegister(0x23, 0x00); //Write 00h into CTRL_REG4;      // Full Scale = +/-2 g
  writeRegister(0x24, 0x00); //Write 08h into CTRL_REG5;      // Default value is 00 for no latching. Interrupt signals on INT1 pin is not latched.
                                                              //Users don’t need to read the INT1_SRC register to clear the interrupt signal.
  // configurations for wakeup and motionless detection
  writeRegister(0x32, 0x10); //Write 10h into INT1_THS;          // Threshold (THS) = 16LSBs * 15.625mg/LSB = 250mg.
  writeRegister(0x33, 0x00); //Write 00h into INT1_DURATION;     // Duration = 1LSBs * (1/10Hz) = 0.1s.
  // readRegister();  //Dummy read to force the HP filter to set reference acceleration/tilt value
  writeRegister(0x30, 0x2A); //Write 2Ah into INT1_CFG;          // Enable XLIE, YLIE, ZLIE interrupt generation, OR logic.
  //lis3dh.setRange(LIS3DH_RANGE_2_G);   // 2, 4, 8 or 16 G!
}


void initHardware() {
  System.enableReset(); //allows System.reset() to work
  pinMode(kBLEConnectedLED, OUTPUT); //BLE connected indicator 
  digitalWrite(kBLEConnectedLED, LOW);
  pinMode(kLIS3DHInterruptPin, INPUT); //LIS3DH interrupt pin
  sleepConfig.mode(SystemSleepMode::ULTRA_LOW_POWER)
    .gpio(kLIS3DHInterruptPin, RISING); //lowest power that does not resume as if from a reset
  
  //while(!Serial.isConnected()){}
  if(!lis3dh.begin(kLis3dhAddress)) {
    delay(1000);
    WITH_LOCK(Serial) {
      Serial.println("Failed to initialize LIS3DH");
    }
  }
  init_ACC();
  wd.runWhileSleeping(true);
  wd.initialize(kWatchDogTimeout);
}

void initFromEEPROM() {
  EEPROM.get(kRecordingIntervalEEPROMAddress, recordingInterval);
  EEPROM.get(kDsidEEPROMAddress, dsid);
  EEPROM.get(kReportingIntervalEEPROMAddress, reportingInterval);
  EEPROM.get(kSleepPauseDurationEEPROMAddress, sleepPauseDuration);
  EEPROM.get(kReportingModeEEPROMAddress, reportingMode);
  reportingInterval = reportingInterval / 1000; // convert to seconds from milliseconds 
  Serial.printlnf("recordingInterval: %i", recordingInterval);
  Serial.printlnf("reportingInterval: %i", reportingInterval);
  Serial.printlnf("sleepPauseDuration: %i", sleepPauseDuration);
  Serial.printlnf("reportingMode: %i", reportingMode);
  if(recordingInterval == kEEPROMEmptyValue) { // if no value stored in EEPROM, set to default
    recordingInterval = kDefaultRecordingInterval; //default value
    // EEPROM.put(kRecordingIntervalEEPROMAddress, recordingInterval);
  }
  if(reportingInterval == kEEPROMEmptyValue) {
    reportingInterval = kDefaultReportingInterval; //default value
  }
  if(sleepPauseDuration == kEEPROMEmptyValue) {
    sleepPauseDuration = kDefaultSleepPauseDuration + 1; //default value
    // EEPROM.put(kSleepPauseDurationEEPROMAddress, sleepPauseDuration);
  }
  if(reportingMode == kEEPROMEmptyValue) {
    reportingMode = kDefaultReportingMode; //default value
    // EEPROM.put(kReportingModeEEPROMAddress, reportingMode);
  }
  if(dsid == kEEPROMEmptyValue) {
    Serial.println("DSID not stored in EEPROM. BLE config required"); 
    // dsid = 51509;
    // EEPROM.put(kDsidEEPROMAddress, dsid);
    //TODO notify user somehow
  }
}