#pragma once
#include "globalVariables.hpp"
#include "constants.hpp"
#include "http.hpp"

void syncSystemTime() {
  // int WiFiConnectCountdown = kWiFiConnectionTimeout;

  // WiFi.on();
  // WiFi.connect();
  // //wait for WiFi to connect for kWiFiConnectionTimeout
  // while(!WiFi.ready() && WiFiConnectCountdown != 0) {
  //   WiFiConnectCountdown = WiFiConnectCountdown - kWiFiCheckInterval;
  //   delay(kWiFiCheckInterval);
  //   WITH_LOCK(Serial) {
  //     Serial.println(WiFiConnectCountdown);
  //   }
  // }
  // if(WiFi.ready()) {
  //   WITH_LOCK(Serial){
  //     Serial.println("WiFi connected, syncing time");
  //   }
  //   Particle.connect();
  //   while(!Particle.connected()) {} // wait forever until cloud connects
  //   Particle.syncTime(); // is async
  //   while(Particle.syncTimePending()) { // wait for syncTime to complete
  //     Particle.process();
  //   }
  //   WITH_LOCK(Serial){
  //     Serial.printlnf("Current time is: %s", Time.timeStr().c_str());
  //   }
  // }
  // else {
  //   WITH_LOCK(Serial) {
  //     Serial.println("WiFi failed to connect, skipping time synchronization");
  //   }
  // }

  // WiFi.off();

  getTime();
}