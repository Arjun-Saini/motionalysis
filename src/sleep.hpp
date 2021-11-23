#include "globalVariables.hpp"
#include "constants.hpp"
#include "Particle.h"

bool sleepReadyTest(){
  if(sleepTimeoutCounter >= sleepPauseDuration / recordingInterval){
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
}