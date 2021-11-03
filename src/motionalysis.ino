SYSTEM_MODE(MANUAL)
SYSTEM_THREAD(ENABLED)
/*
 * Project motionalysis-testFirmware
 * Description: Attempting to diagnose Motionalysis issues
 * Author: trylaarsdam
 * Date:
 */
#include "HttpClient/HttpClient.h"
#include "constants.hpp"

int recordingInterval; // interval between lis3dh reads
int reportingInterval; // interval between reporting data to server in seconds
String payload = "";
bool valuesChanged = false;
String unixTime;
String ssid, password = "";
float x, y, z;
uint8_t storedValues [256];
long storedTimes [256];
float prevX, prevY, prevZ;
int storedValuesIndex = 0;
enum firmwareStateEnum {
  BLEWAIT,
  RECORDING,
  SENDING
};
uint8_t firmwareState = BLEWAIT;
bool bleWaitForConfig = false; //when true, firmware is waiting for user input over BLE b/c BLE was connected
String bleInputBuffer; // buffer for reading from BLE and writing to EEPROM
int bleQuestionCount, dsid, size;
bool waitingForOTA = false;

//setup for http connection
HttpClient http;
http_header_t headers[] = {
  {"Accept", "application/json"},
  {"Content-Type", "application/json"},
  {NULL, NULL}
};
http_request_t request;
http_response_t response;


int networkCount;
WiFiAccessPoint networks[5];
String networkBuffer;

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);

void reportingThread(void* args);
os_thread_t reportingThreadHandle;
os_mutex_t payloadAccessLock;

void initFromEEPROM() {
  EEPROM.get(kRecordingIntervalEEPROMAddress, recordingInterval);
  EEPROM.get(kDsidEEPROMAddress, dsid);
  EEPROM.get(kReportingIntervalEEPROMAddress, reportingInterval);
  reportingInterval = reportingInterval / 1000; // convert to seconds from milliseconds 
  Serial.printlnf("recordingInterval: %i", recordingInterval);
  Serial.printlnf("reportingInterval: %i", reportingInterval);
  if(recordingInterval == kEEPROMEmptyValue) { // if no value stored in EEPROM, set to default
    recordingInterval = kDefaultRecordingInterval; //default value
  }
  if(reportingInterval == kEEPROMEmptyValue) {
    reportingInterval = kDefaultReportingInterval; //default value
  }
  if(dsid == kEEPROMEmptyValue) {
    Serial.println("DSID not stored in EEPROM. BLE config required"); 
    //TODO notify user somehow
  }
}

void syncSystemTime() {
  int WiFiConnectCountdown = kWiFiConnectionTimeout;

  WiFi.on();
  WiFi.connect();
  //wait for WiFi to connect for kWiFiConnectionTimeout
  while(!WiFi.ready() && WiFiConnectCountdown <= 0) {
    WiFiConnectCountdown = WiFiConnectCountdown - kWiFiCheckInterval;
    delay(kWiFiCheckInterval);
  }
  if(WiFi.ready()) {
    Serial.println("WiFi connected, syncing time");
    Particle.connect();
    while(!Particle.connected()) {} // wait forever until cloud connects
    Particle.syncTime(); // is async
    while(Particle.syncTimePending()) { // wait for syncTime to complete
      Particle.process();
    }
    Serial.printlnf("Current time is: %s", Time.timeStr().c_str());
  }
  else {
    Serial.println("WiFi failed to connect, skipping time synchronization");
  }

  WiFi.off();
}

void HTTPRequestSetup() {
  request.hostname = kHTTPHostname;
  request.port = kHTTPRequestPort;
  request.path = "/";
}

void initHardware() {
  System.enableReset(); //allows System.reset() to work
  pinMode(kBLEConnectedLED, OUTPUT); //BLE connected indicator 
  digitalWrite(kBLEConnectedLED, LOW);
  if(!lis3dh.begin(kLis3dhAddress)) {
    delay(1000);
    WITH_LOCK(Serial) {
      Serial.println("Failed to initialize LIS3DH");
    }
  }
}

// setup() runs once, when the device is first turned on.
void setup() {
  Serial.begin(9600);
  while(!Serial.isConnected()){} //TODO REMOVE BEFORE RELEASE

  initHardware();
  HTTPRequestSetup();
  initFromEEPROM();
  syncSystemTime();

  os_mutex_create(&payloadAccessLock);
  os_thread_create(&reportingThreadHandle, "reportThread", OS_THREAD_PRIORITY_DEFAULT, reportingThread, NULL, 1024);
}

// loop() runs over and over again, as quickly as it can execute.
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
        storedValuesIndex++; // TODO re-add payload creation
        os_mutex_lock(&payloadAccessLock);
        //payload = whatever
        os_mutex_unlock(&payloadAccessLock);
      }
      else {
        firstLIS3DHReading = false;
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

void reportData(String payload) {
  WiFi.on();
  WiFi.connect();
  while(!WiFi.ready()) {
    delay(100);
  }
  if(WiFi.ready() != true) {
    WITH_LOCK(Serial) {
      Serial.println("WiFi failed to connect, data not reported");
    }
  }
  else {
    WITH_LOCK(Serial) {
      Serial.println("WiFi connected, reporting data");
    }
    payload.remove(payload.length() - 1);
    request.body = "{\"data\":[" + payload + "]}";
    http.post(request, response, headers);
    WITH_LOCK(Serial) {
      Serial.println("Status: " + response.status);
      Serial.println("Body: " + response.body);
      Serial.println("ReqBody: " + request.body);
    }
  }
  WiFi.off();
}

void reportingThread(void *args) {
  while(true) {
    if(storedValuesIndex >= ((reportingInterval * kSecondsToMilliseconds) / recordingInterval)) {
      WITH_LOCK(Serial) {
        Serial.println("reporting");
      }
      os_mutex_lock(&payloadAccessLock); // lock access to payload before copying to local variable and resetting global payload
      String localPayload = payload;
      payload = "";
      os_mutex_unlock(&payloadAccessLock);
      reportData(localPayload);
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
        goto SSID;
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