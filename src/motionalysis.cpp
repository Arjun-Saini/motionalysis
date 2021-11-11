/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/trylaarsdam/Documents/dev/motionalysis/src/motionalysis.ino"
void setup();
void loop();
void reportingThread(void *args);
void connectCallback(const BlePeerDevice& peer, void* context);
void disconnectCallback(const BlePeerDevice& peer, void* context);
#line 1 "/Users/trylaarsdam/Documents/dev/motionalysis/src/motionalysis.ino"
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


void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);

void reportingThread(void* args);
os_thread_t reportingThreadHandle;
os_mutex_t payloadAccessLock;

// setup() runs once, when the device is first turned on.
void setup() {
  Serial.begin(9600);
  while(!Serial.isConnected()){}
  initHardware();
  HTTPRequestSetup(); 
  initFromEEPROM();
  syncSystemTime();

  os_mutex_create(&payloadAccessLock);
  os_mutex_unlock(&payloadAccessLock);
  os_thread_create(&reportingThreadHandle, "reportThread", OS_THREAD_PRIORITY_DEFAULT, reportingThread, NULL, 1024);
}

bool firstLIS3DHReading = true; //sets first recorded value to 0
void loop() {
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
        } else {
          storedValues[storedValuesIndex] = 0;
        }
        storedTimes[storedValuesIndex] = Time.now(); 
        storedValuesIndex++; 
        WITH_LOCK(Serial) {
          Serial.println("requesting payloadAccessLock");
        }
        
        os_mutex_lock(payloadAccessLock);
        //Lock the recording loop until reporting thread has completed making the payload
        //if lock was not present there could be data that was not reported if recordingDelay was low enough
        delay(100);
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
      String localPayload = payload;
      payload = "";
      os_mutex_unlock(payloadAccessLock);
      reportData(localPayload);
      storedValuesIndex = 0;
    }
    os_thread_yield();
  }
}

//ble interface
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context){
  bleInputBuffer = "";

  //each case is a separate prompt
  switch(bleQuestionCount){
    case 0:{
      //ssid prompt
      SSID:
      txCharacteristic.setValue("\nCredentials are currently stored for:\n[");
      networkCount = WiFi.getCredentials(networks, 5);
      for(int i = 0; i < networkCount - 1; i++){
        networkBuffer = networks[i].ssid;
        WITH_LOCK(Serial) {
          Serial.println(networkBuffer.length());
        }
        txCharacteristic.setValue(networkBuffer);
        txCharacteristic.setValue(",\n");
      }
      networkBuffer = networks[networkCount - 1].ssid;
      WITH_LOCK(Serial) {
        Serial.println(networkBuffer.length());
      }
      txCharacteristic.setValue(networkBuffer);
      txCharacteristic.setValue("]\nEnter network SSID (blank to skip, 'clear' to reset credentials): ");
      break;
    }
    case 1:{
      //store ssid
      for(int i = 0; i < len - 1; i++){
        bleInputBuffer += (char)data[i];
        ssid = bleInputBuffer;
        WITH_LOCK(Serial) {
          Serial.println(data[i]);
        }
      }
      WITH_LOCK(Serial) {
        Serial.println(ssid);
        Serial.println(ssid.length());
      }
      if(ssid == ""){
        //dsid prompt if wifi prompt skipped
        bleQuestionCount = 3;
        EEPROM.get(0, dsid);
        txCharacteristic.setValue("\nCurrent DSID is [");
        if(dsid != -1){
          txCharacteristic.setValue(String(dsid));
        }
        txCharacteristic.setValue("]\nEnter device DSID (blank to skip): ");
      }else if(ssid == "clear"){
        //go back to ssid prompt if credentials are cleared
        WiFi.clearCredentials();
        bleQuestionCount = 0;
        goto SSID; //TODO let kevin have fun fixing this
      }else{
        //password prompt if ssid entered
        txCharacteristic.setValue("\nEnter network password: ");
      }
      break;
    }
    case 2:{
      //save credentials with given ssid and password
      for(int i = 0; i < len - 1; i++){
        WITH_LOCK(Serial) {
          Serial.println(data[i]);
        }
        bleInputBuffer += (char)data[i];
        password = bleInputBuffer;
      }
      WITH_LOCK(Serial) {
        Serial.println(password);
        Serial.println(password.length());
      }
      WiFi.setCredentials(ssid, password);
      WITH_LOCK(Serial) {
        Serial.println("\n\nCredentials set with ssid: " + ssid + "\npassword: " + password + "\n\n");
      }

      //wifi test prompt
      txCharacteristic.setValue("\nEnter 'test' to test credentials (blank to skip): ");
      break;
    }
    case 3:{
      for(int i = 0; i < len - 1; i++){
        WITH_LOCK(Serial) {
          Serial.println(data[i]);
        }
        bleInputBuffer += (char)data[i];
      }
      if(bleInputBuffer == "test" && WiFi.hasCredentials()){
        //attempt to connect with credentials, times out if unsuccessful
        bool wifiTest = true;
        int wifiTimeout = 20000;
        WiFi.on();
        WiFi.connect();
        while(WiFi.connecting() || !WiFi.ready()){
          if(wifiTimeout >= 20000){
            wifiTest = false;
            Serial.println("timeout");
            break;
          }
          wifiTimeout = wifiTimeout + 100;
          delay(100);
        }
        WiFi.off();
        if(wifiTest){
          txCharacteristic.setValue("Success!\n");
        }else{
          //go back to ssid prompt if test failed
          txCharacteristic.setValue("ERROR: WiFi connection timeout\n");
          bleQuestionCount = 0;
          wifiTest = true;
          goto SSID;
        }
      }
      //dsid prompt
      EEPROM.get(0, dsid);
      txCharacteristic.setValue("\nCurrent DSID is [");
      if(dsid != -1){
        txCharacteristic.setValue(String(dsid));
      }
      txCharacteristic.setValue("]\nEnter device DSID (blank to skip): ");
      break;
    }
    case 4:{
      //store dsid in eeprom
      for(int i = 0; i < len - 1; i++){
        WITH_LOCK(Serial) {
          Serial.println(data[i]);
        }
        bleInputBuffer += (char)data[i];
        dsid = atoi(bleInputBuffer);
      }
      if(bleInputBuffer != ""){
        EEPROM.put(0, dsid);
        WITH_LOCK(Serial) {
          Serial.println("dsid entered");
        }
      }
      EEPROM.get(0, dsid);
      Serial.println("dsid: " + dsid);
      EEPROM.get(100, recordingInterval);

      //prompt for data collection interval
      txCharacteristic.setValue("\nCurrent value for data collection interval is [");
      if(recordingInterval != -1){
        txCharacteristic.setValue(String(recordingInterval));
      }
      txCharacteristic.setValue("]\nEnter time between data collection as an integer in milliseconds (blank to skip): ");
      break;
    }
    case 5:{
      //store data collection interval in eeprom
      for(int i = 0; i < len - 1; i++){
        WITH_LOCK(Serial) {
          Serial.println(data[i]);
        }
        bleInputBuffer += (char)data[i];
        recordingInterval = atoi(bleInputBuffer);
      }
      if(bleInputBuffer == ""){
        EEPROM.get(100, recordingInterval);
      }
      EEPROM.put(100, recordingInterval);
      EEPROM.get(100, recordingInterval);
      //Serial.println(recordingInterval);
      EEPROM.get(200, reportingInterval);

      //prompt for wifi connection interval
      txCharacteristic.setValue("\nCurrent value for WiFi connection interval is [");
      if(reportingInterval != -1){
        txCharacteristic.setValue(String(reportingInterval / 1000));
      }
      txCharacteristic.setValue("]\nEnter time between WiFi connections as an integer in seconds (blank to skip): ");
      break;
    }
    case 6:{
      //store wifi connection interval in eeprom
      for(int i = 0; i < len - 1; i++){
        WITH_LOCK(Serial) {
          Serial.println(data[i]);
        }
        bleInputBuffer += (char)data[i];
        reportingInterval = atoi(bleInputBuffer) * 1000;
      }
      if(bleInputBuffer == ""){
        EEPROM.get(200, reportingInterval);
      }
      EEPROM.put(200, reportingInterval);
      EEPROM.get(200, reportingInterval);
      WITH_LOCK(Serial) {
        Serial.println(reportingInterval);
      }

      //prompt for ota
      txCharacteristic.setValue("\nEnter 'ota' to wait for OTA update (blank to skip): ");
      break;
    }
    case 7:{
      //enter ota mode if command entered
      for(int i = 0; i < len - 1; i++){
        WITH_LOCK(Serial) {
          Serial.println(data[i]);
        }
        bleInputBuffer += (char)data[i];
      }
      if(bleInputBuffer == "ota"){
        System.updatesEnabled();
        waitingForOTA = true;
      }

      //causes data collection to begin
      //firmwareState = RECORDING;
      if(waitingForOTA) {
        System.updatesEnabled();
        WiFi.on();
        WiFi.connect();
        while(!WiFi.ready()) {
          delay(100);
        }
        if(WiFi.ready() != true) {
          WITH_LOCK(Serial) {
            Serial.println("WiFi failed to connect, skipping time synchronization");
          }
        }
        else {
          WITH_LOCK(Serial) {
            Serial.println("WiFi connected, awaiting update");
          }
          txCharacteristic.setValue("\nAwaiting OTA update");
          Particle.connect();
          while(!Particle.connected()) {
            //Particle.process();
            delay(100);
          }
          while(1){
            Particle.process();
          }
        }
      }
      System.reset();
    }
  }

  bleQuestionCount++;
}

//kBLEConnectedLED turns on when ble connected
void connectCallback(const BlePeerDevice& peer, void* context){
  bleQuestionCount = 0;
  WITH_LOCK(Serial) {
    Serial.println("connected");
  }
  digitalWrite(kBLEConnectedLED, HIGH);
}

//kBLEConnectedLED turns off when ble disconnected
void disconnectCallback(const BlePeerDevice& peer, void* context){
  WITH_LOCK(Serial) {
    Serial.println("disconnected");
  }
  digitalWrite(kBLEConnectedLED, LOW);
}