SYSTEM_MODE(MANUAL)

/*
 * Project motionalysis-testFirmware
 * Description: Attempting to diagnose Motionalysis issues
 * Author: trylaarsdam
 * Date:
 */
#include <Adafruit_LIS3DH.h>
#include "HttpClient/HttpClient.h"


Adafruit_LIS3DH lis3dh = Adafruit_LIS3DH();
int recordingInterval; // interval between lis3dh reads
int reportingInterval; // interval between reporting data to server in seconds
String payload, prevPayload = "";
bool valuesChanged = false;
String unixTime;
String ssid, password = "";
float x, y, z;
uint8_t storedValues [256];
long storedTimes [256];
float prevX, prevY, prevZ;
int storedValuesPos = 0;
enum firmwareStateEnum {
  BLEWAIT,
  RECORDING,
  SENDING
};
uint8_t firmwareState = BLEWAIT;
bool bleWaitForConfig = false;
String inputBuffer;
int count, dsid, size;
bool ota = false;

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


// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  System.enableReset();
  request.hostname = "digiglue.io";
  request.port = 80;
  request.path = "/";
  Serial.begin(9600);
  if(!lis3dh.begin(0x18)) {
    delay(1000);
    Serial.println("Failed to initialize LIS3DH");
  }
  //Wire.end();

  EEPROM.get(100, recordingInterval);
  EEPROM.get(0, dsid);
  Serial.println(recordingInterval);
  EEPROM.get(200, reportingInterval);
  reportingInterval = reportingInterval / 1000; // convert to seconds from milliseconds 
  Serial.printlnf("recordingInterval: %i", recordingInterval);
  Serial.printlnf("reportingInterval: %i", reportingInterval);
  if(recordingInterval == -1) {
    recordingInterval = 500; //default value
  }
  if(reportingInterval == -1) {
    reportingInterval = 10; //default value
  }
  int WiFiConnectCountdown = 20000;
  //sync time
  WiFi.on();
  WiFi.connect();
  while(!WiFi.ready() && WiFiConnectCountdown != 0) {
    WiFiConnectCountdown= WiFiConnectCountdown - 100;
    delay(100);
  }
  if(WiFi.ready() != true) {
    Serial.println("WiFi failed to connect, skipping time synchronization");
  }
  else {
    Serial.println("WiFi connected, syncing time");
    Particle.connect();
    while(!Particle.connected()) {
      //Particle.process();
      delay(100);
    }
    Particle.syncTime();
    while(Particle.syncTimePending()) {
      Particle.process();
    }
    Serial.printlnf("Current time is: %s", Time.timeStr().c_str());
  }
  WiFi.off();
}
// loop() runs over and over again, as quickly as it can execute.
bool firstRecord = true; //sets first value to 0
void loop() {
  switch (firmwareState) {
    case BLEWAIT: {
      //wait for BLE connection
      Serial.println("BLEWAIT");
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
        Serial.println(BLECountdown);
        delay(10);
      }

      if(BLE.connected()){ 
        Serial.println("BLE connected");
        bleWaitForConfig = true;
      }
      else {
        bleWaitForConfig = false;
        Serial.println("BLE not connected, continuing with stored settings.");
      }

      while(bleWaitForConfig) {
        Serial.println("bleWaitForConfig");
        delay(100);
      }
      BLE.disconnectAll();
      BLE.off();
      firmwareState = RECORDING;
      break;
    }
    case RECORDING: {
      //record data
      lis3dh.read();
      x = lis3dh.x_g;
      y = lis3dh.y_g;
      z = lis3dh.z_g;
      if(abs(x - prevX) > 0.05 || abs(y - prevY) > 0.05 || abs(z - prevZ) > 0.05) {
        //Serial.println("movement above threshold");
        if(firstRecord) {
          firstRecord = false;
          storedValues[storedValuesPos] = 0; //otherwise first record is always 1
        }
        else {
          storedValues[storedValuesPos] = 1;
        }
      }
      else {
        storedValues[storedValuesPos] = 0;
      }
      //Time.setFormat(TIME_FORMAT_UNIX);
      storedTimes[storedValuesPos] = Time.now();
      prevX = x;
      prevY = y;
      prevZ = z;
      storedValuesPos++;
      if(storedValuesPos >= ((reportingInterval * 1000) / recordingInterval)) {
        for (int i = 0; i < storedValuesPos; i++) {
          //Serial.printf("{timestamp: %i, data: %i}, ", storedTimes[i], storedValues[i]);
          payload += "{\"dsid\":" + String(dsid) + ", \"value\":" + storedValues[i] + ", \"timestamp\":" + String(storedTimes[i]) + "},";
        }
        Serial.println("\n");
        storedValuesPos = 0;
        int WiFiConnectCountdown = 20000;
        //sync time
        WiFi.on();
        WiFi.connect();
        while(!WiFi.ready() && WiFiConnectCountdown != 0) {
          WiFiConnectCountdown= WiFiConnectCountdown - 100;
          delay(100);
        }
        if(WiFi.ready() != true) {
          Serial.println("WiFi failed to connect, data not reported");
        }
        else {
          Serial.println("WiFi connected, reporting data");
          payload.remove(payload.length() - 1);
          request.body = "{\"data\":[" + payload + "]}";
          payload = "";
          http.post(request, response, headers);
          Serial.println("Status: " + response.status);
          Serial.println("Body: " + response.body);
          Serial.println("ReqBody: " + request.body);

        }
        WiFi.off();
      }
      delay(recordingInterval);
      break;
    }
    case SENDING: {
      //send data to server, not used 
      break;
    }
  }
}


//ble interface
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context){
  Serial.println(len);
  inputBuffer = "";

  //each case is a separate prompt
  switch(count){
    case 0:{
      //ssid prompt
      SSID:
      txCharacteristic.setValue("\nCredentials are currently stored for:\n[");
      networkCount = WiFi.getCredentials(networks, 5);
      for(int i = 0; i < networkCount - 1; i++){
        networkBuffer = networks[i].ssid;
        Serial.println(networkBuffer.length());
        txCharacteristic.setValue(networkBuffer);
        txCharacteristic.setValue(",\n");
      }
      networkBuffer = networks[networkCount - 1].ssid;
      Serial.println(networkBuffer.length());
      txCharacteristic.setValue(networkBuffer);
      txCharacteristic.setValue("]\nEnter network SSID (blank to skip, 'clear' to reset credentials): ");
      break;
    }
    case 1:{
      //store ssid
      for(int i = 0; i < len - 1; i++){
        inputBuffer += (char)data[i];
        ssid = inputBuffer;
        Serial.println(data[i]);
      }
      Serial.println(ssid);
      Serial.println(ssid.length());
      if(ssid == ""){
        //dsid prompt if wifi prompt skipped
        count = 3;
        EEPROM.get(0, dsid);
        txCharacteristic.setValue("\nCurrent DSID is [");
        if(dsid != -1){
          txCharacteristic.setValue(String(dsid));
        }
        txCharacteristic.setValue("]\nEnter device DSID (blank to skip): ");
      }else if(ssid == "clear"){
        //go back to ssid prompt if credentials are cleared
        WiFi.clearCredentials();
        count = 0;
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
        Serial.println(data[i]);
        inputBuffer += (char)data[i];
        password = inputBuffer;
      }
      Serial.println(password);
      Serial.println(password.length());
      WiFi.setCredentials(ssid, password);
      Serial.println("\n\nCredentials set with ssid: " + ssid + "\npassword: " + password + "\n\n");

      //wifi test prompt
      txCharacteristic.setValue("\nEnter 'test' to test credentials (blank to skip): ");
      break;
    }
    case 3:{
      for(int i = 0; i < len - 1; i++){
        Serial.println(data[i]);
        inputBuffer += (char)data[i];
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
        Serial.println(data[i]);
        inputBuffer += (char)data[i];
        dsid = atoi(inputBuffer);
      }
      if(inputBuffer != ""){
        EEPROM.put(0, dsid);
        Serial.println("dsid entered");
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
        Serial.println(data[i]);
        inputBuffer += (char)data[i];
        recordingInterval = atoi(inputBuffer);
      }
      if(inputBuffer == ""){
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
        Serial.println(data[i]);
        inputBuffer += (char)data[i];
        reportingInterval = atoi(inputBuffer) * 1000;
      }
      if(inputBuffer == ""){
        EEPROM.get(200, reportingInterval);
      }
      EEPROM.put(200, reportingInterval);
      EEPROM.get(200, reportingInterval);
      Serial.println(reportingInterval);

      //prompt for ota
      txCharacteristic.setValue("\nEnter 'ota' to wait for OTA update (blank to skip): ");
      break;
    }
    case 7:{
      //enter ota mode if command entered
      for(int i = 0; i < len - 1; i++){
        Serial.println(data[i]);
        inputBuffer += (char)data[i];
      }
      if(inputBuffer == "ota"){
        System.updatesEnabled();
        ota = true;
      }

      //causes data collection to begin
      //firmwareState = RECORDING;
      if(ota) {
        System.updatesEnabled();
        WiFi.on();
        WiFi.connect();
        while(!WiFi.ready()) {
          delay(100);
        }
        if(WiFi.ready() != true) {
          Serial.println("WiFi failed to connect, skipping time synchronization");
        }
        else {
          Serial.println("WiFi connected, awaiting update");
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
      //digitalWrite(D7, LOW);
    }
  }

  count++;
}

//d7 led turns on when ble connected
void connectCallback(const BlePeerDevice& peer, void* context){
  count = 0;
  Serial.println("connected");
  digitalWrite(D7, HIGH);
}

//d7 led turns off when ble disconnected
void disconnectCallback(const BlePeerDevice& peer, void* context){
  Serial.println("disconnected");
  digitalWrite(D7, LOW);
}