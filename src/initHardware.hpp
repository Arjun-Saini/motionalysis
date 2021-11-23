#pragma once
#include "globalVariables.hpp"
#include "constants.hpp"
#include <Adafruit_LIS3DH.h>

Adafruit_LIS3DH lis3dh = Adafruit_LIS3DH();

void initHardware() {
  System.enableReset(); //allows System.reset() to work
  pinMode(kBLEConnectedLED, OUTPUT); //BLE connected indicator 
  digitalWrite(kBLEConnectedLED, LOW);
  pinMode(kLIS3DHInterruptPin, INPUT); //LIS3DH interrupt pin
  sleepConfig.mode(SystemSleepMode::ULTRA_LOW_POWER)
    .gpio(kLIS3DHInterruptPin, FALLING); //lowest power that does not resume as if from a reset
  

  if(!lis3dh.begin(kLis3dhAddress)) {
    delay(1000);
    WITH_LOCK(Serial) {
      Serial.println("Failed to initialize LIS3DH");
    }
  }
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