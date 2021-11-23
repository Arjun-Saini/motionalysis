#pragma once
#include "globalVariables.hpp"
#include "constants.hpp"
#include "Particle.h"

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
    Serial.println("Sleep process waiting for mutex locks");
  }
  os_mutex_lock(reportingSleepProtectionLock);
  System.sleep(sleepConfig);
  sleepTimeoutCounter = 0;
  os_mutex_unlock(reportingSleepProtectionLock);
}