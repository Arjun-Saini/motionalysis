SYSTEM_MODE(MANUAL)
SYSTEM_THREAD(ENABLED)
/*
 * Project motionalysis-testFirmware
 * Description: Attempting to diagnose Motionalysis issues
 * Author: trylaarsdam
 * Date:
 */
#include "constants.hpp"
#include "http.hpp"
#include "initHardware.hpp"
#include "systemSync.hpp"
#include "globalVariables.hpp"
#include "motionalysis.hpp"
#include "ble.hpp"
#include "sleep.hpp"

void reportingThread(void* args);


// setup() runs once, when the device is first turned on.
void setup() {
  Serial.begin(9600);
  //while(!Serial.isConnected()){} 
  initHardware();
  HTTPRequestSetup(); 
  initFromEEPROM();
  syncSystemTime();

  os_mutex_create(&payloadAccessLock);
  os_mutex_create(&reportingSleepProtectionLock);
  os_mutex_unlock(&reportingSleepProtectionLock);
  os_mutex_unlock(&payloadAccessLock);
  os_thread_create(&reportingThreadHandle, "reportThread", OS_THREAD_PRIORITY_DEFAULT, reportingThread, NULL, 1024);
}

bool firstLIS3DHReading = true; //sets first recorded value to 0
void loop() {
  wd.pet();
  switch (firmwareState) {
    case BLEWAIT: {
      //wait for BLE connection
      WITH_LOCK(Serial) {
        Serial.println("BLEWAIT");
      }
      BLE.on();
      BLE.addCharacteristic(txCharacteristic);
      BLE.addCharacteristic(rxCharacteristic);

      BleAdvertisingData data;
      data.appendServiceUUID(serviceUuid);
      BLE.advertise(&data);
      BLE.onConnected(connectCallback);
      BLE.onDisconnected(disconnectCallback);
      int BLECountdown = 5000;
      while(!BLE.connected() && BLECountdown > 0) {
        BLECountdown = BLECountdown - 10;
        WITH_LOCK(Serial) {
          Serial.println(BLECountdown);
        }
        delay(10);
      }

      if(BLE.connected()){ 
        WITH_LOCK(Serial) {
          Serial.println("BLE connected");
        }
        bleWaitForConfig = true;
      }
      else {
        bleWaitForConfig = false;
        WITH_LOCK(Serial) {
          Serial.println("BLE not connected, continuing with stored settings.");
        }
      }

      while(bleWaitForConfig) {
        WITH_LOCK(Serial) {
          Serial.println("bleWaitForConfig");
        }
        delay(100);
      }
      BLE.disconnectAll();
      BLE.off();
      firmwareState = RECORDING;
      break;
    }
    case RECORDING: {
      //record data
      WITH_LOCK(Serial) {
        Serial.println("RECORDING");
      }
      lis3dh.read();
      x = lis3dh.x_g;
      y = lis3dh.y_g;
      z = lis3dh.z_g;
      if(!firstLIS3DHReading) {
        if(abs(x - prevX) > kDeltaAccelThreshold || abs(y - prevY) > kDeltaAccelThreshold || abs(z - prevZ) > kDeltaAccelThreshold) {
          storedValues[storedValuesIndex] = 1;
          sleepTimeoutCounter = 0; // reset sleep timeout because movement detected
        } else {
          storedValues[storedValuesIndex] = 0;
          if(storedValues[storedValuesIndex - 1] == 0) {
            sleepTimeoutCounter++;
            if(sleepReadyTest()){
              engageSleep();
            }
          }
        }
        storedTimes[storedValuesIndex] = Time.now(); 
        WITH_LOCK(Serial) {
          Serial.printlnf("Recording index: %i", storedValuesIndex);
        }
        storedValuesIndex++; 
        WITH_LOCK(Serial) {
          Serial.println("requesting payloadAccessLock");
        }
        os_mutex_lock(payloadAccessLock);
        //Lock the recording loop until reporting thread has completed making the payload
        //if lock was not present there could be data that was not reported if recordingDelay was low enough
        delay(1);
        os_mutex_unlock(payloadAccessLock);
        WITH_LOCK(Serial) {
          Serial.println("payloadAccessLock released by RECORDING");
        }
      }
      else {
        firstLIS3DHReading = false;
        WITH_LOCK(Serial) {
          Serial.println("First reading");
        }
      }
      
      prevX = x;
      prevY = y;
      prevZ = z;
      delay(recordingInterval);
      break;
    }
    case SENDING: {
      //send data to server, not used 
      break;
    }
  }
}

void reportingThread(void *args) {
  while(true) {
    if(storedValuesIndex >= ((reportingInterval * kSecondsToMilliseconds) / recordingInterval)) {

      os_mutex_lock(payloadAccessLock); // lock access to payload before copying to local variable and resetting global payload
      for (int i = 0; i < storedValuesIndex; i++) {
        //Serial.printf("{timestamp: %i, data: %i}, ", storedTimes[i], storedValues[i]);
        payload += "{\"dsid\":" + String(dsid) + ", \"value\":" + storedValues[i] + ", \"timestamp\":" + String(storedTimes[i]) + "},";
      }
      storedValuesIndex = 0;
      String localPayload = payload;
      payload = "";
      os_mutex_unlock(payloadAccessLock);
      os_mutex_lock(reportingSleepProtectionLock);
      reportData(localPayload);
      os_mutex_unlock(reportingSleepProtectionLock);
      init_ACC();
    }
    os_thread_yield();
  }
}
