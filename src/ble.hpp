#pragma once
#include "constants.hpp"
#include "http.hpp"
#include "initHardware.hpp"
#include "systemSync.hpp"
#include "globalVariables.hpp"

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);

//ble interface
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context){
  bleInputBuffer = "";

  //each case is a separate prompt
  switch(bleQuestionCount){
    case 0:{
      //ssid prompt
      SSID:
      txCharacteristic.setValue("\nv5.1 Motionalysis Firmware\nCredentials are currently stored for:\n[");
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
        EEPROM.put(kDsidEEPROMAddress, dsid);
        WITH_LOCK(Serial) {
          Serial.println("dsid entered");
        }
      }
      EEPROM.get(kDsidEEPROMAddress, dsid);
      Serial.println("dsid: " + dsid);

      //prompt for data collection interval
      txCharacteristic.setValue("\nCurrent value for sleep pause duration is [");
      if(recordingInterval != -1){
        txCharacteristic.setValue(String(sleepPauseDuration));
      }
      txCharacteristic.setValue("]\nEnter sleep pause duration as an integer in seconds (blank to skip): ");
      break;
    }
    case 5:{
      //store dsid in eeprom
      for(int i = 0; i < len - 1; i++){
        WITH_LOCK(Serial) {
          Serial.println(data[i]);
        }
        bleInputBuffer += (char)data[i];
        sleepPauseDuration = atoi(bleInputBuffer);
      }
      if(bleInputBuffer != ""){
        EEPROM.put(kSleepPauseDurationEEPROMAddress, sleepPauseDuration);
        WITH_LOCK(Serial) {
          Serial.println("sleep pause duration entered");
        }
      }
      EEPROM.get(kSleepPauseDurationEEPROMAddress, sleepPauseDuration);
      Serial.println("sleep pause duration: " + sleepPauseDuration);
      EEPROM.get(kRecordingIntervalEEPROMAddress, recordingInterval);

      //prompt for data collection interval
      txCharacteristic.setValue("\nCurrent value for data collection interval is [");
      if(recordingInterval != -1){
        txCharacteristic.setValue(String(recordingInterval));
      }
      txCharacteristic.setValue("]\nEnter time between data collection as an integer in milliseconds (blank to skip): ");
      break;
    }
    case 6:{
      //store data collection interval in eeprom
      for(int i = 0; i < len - 1; i++){
        WITH_LOCK(Serial) {
          Serial.println(data[i]);
        }
        bleInputBuffer += (char)data[i];
        recordingInterval = atoi(bleInputBuffer);
      }
      if(bleInputBuffer == ""){
        EEPROM.get(kRecordingIntervalEEPROMAddress, recordingInterval);
      }
      EEPROM.put(kRecordingIntervalEEPROMAddress, recordingInterval);
      EEPROM.get(kRecordingIntervalEEPROMAddress, recordingInterval);
      //Serial.println(recordingInterval);
      EEPROM.get(kReportingIntervalEEPROMAddress, reportingInterval);

      //prompt for wifi connection interval
      txCharacteristic.setValue("\nCurrent value for WiFi connection interval is [");
      if(reportingInterval != -1){
        txCharacteristic.setValue(String(reportingInterval / 1000));
      }
      txCharacteristic.setValue("]\nEnter time between WiFi connections as an integer in seconds (blank to skip): ");
      break;
    }
    case 7:{
      //store wifi connection interval in eeprom
      for(int i = 0; i < len - 1; i++){
        WITH_LOCK(Serial) {
          Serial.println(data[i]);
        }
        bleInputBuffer += (char)data[i];
        reportingInterval = atoi(bleInputBuffer) * 1000;
      }
      if(bleInputBuffer == ""){
        EEPROM.get(kReportingIntervalEEPROMAddress, reportingInterval);
      }
      EEPROM.put(kReportingIntervalEEPROMAddress, reportingInterval);
      EEPROM.get(kReportingIntervalEEPROMAddress, reportingInterval);
      WITH_LOCK(Serial) {
        Serial.println(reportingInterval);
      }

      //prompt for ota
      txCharacteristic.setValue("\nEnter 'ota' to wait for OTA update (blank to skip): ");
      break;
    }
    case 8:{
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