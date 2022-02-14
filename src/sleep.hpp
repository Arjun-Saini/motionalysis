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
  delay(100);
  System.sleep(sleepConfig);
  sleepTimeoutCounter = 0;
  // init_ACC();
  // os_mutex_unlock(reportingSleepProtectionLock);
}