#pragma once
#include "globalVariables.hpp"
#include "constants.hpp"
#include "Particle.h"
#include "initHardware.hpp"

bool sleepReadyTest(){
  if(sleepTimeoutCounter >= ((sleepPauseDuration * 1000) / recordingInterval)){
    return true;
  }
  else {
    return false;
  }
}

void engageSleep() {
  WITH_LOCK(Serial) {
    Serial.println("Engaging sleep.");
  }
  WITH_LOCK(Serial) {
    Serial.println(">>> CONTINUING REPORTING DATA");
    Serial.printlnf("storedValuesIndex: %i", storedValuesIndex);
  }
  if(restartFromWatchdog) {
    delay(100);
    System.sleep(sleepConfig);
    sleepTimeoutCounter = 0;
  }
  else {
    for (int i = 0; i < storedValuesIndex; i++) {
      //Serial.printf("{timestamp: %i, data: %i}, ", storedTimes[i], storedValues[i]);
      payload += "{\"dsid\":" + String(dsid) + ", \"value\":" + storedValues[i] + ", \"timestamp\":" + String(storedTimes[i]) + "},";
    }
    storedValuesIndex = 0;
    String localPayload = payload;
    payload = "";

    reportData(localPayload);

    while(WiFi.isOn()) {}
    // init_ACC();

    delay(100);
    System.sleep(sleepConfig);
    sleepTimeoutCounter = 0;
    // init_ACC();
    // os_mutex_unlock(reportingSleepProtectionLock);
  }
  
}