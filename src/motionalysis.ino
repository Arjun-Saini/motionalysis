SYSTEM_MODE(MANUAL)

#include "Adafruit_LIS3DH.h"
#include "HttpClient/HttpClient.h"
#include <string>

#define I2C_ADDRESS 0x19
#define GRAVITY 9.8066
#define SDO_OUTPUT_PIN D8
#define CONFIG_WAIT_TIME 30000
#define DEFAULT_SLEEP_DURATION 1000
#define DEFAULT_WIFI_INTERVAL 60000
#define SLEEP_DELAY 70
#define WIFI_TEST_TIMEOUT 30000
#define ROUNDING_FACTOR 10

int sleepDuration = DEFAULT_SLEEP_DURATION;
int wifiInterval = DEFAULT_WIFI_INTERVAL;

String payload = "";
int wifiTimeLeft;
float x, y, z;
String unixTime;
int isMoving;
String ssid, password = "";

Adafruit_LIS3DH lis = Adafruit_LIS3DH();
SystemSleepConfiguration config;

//setup for http connection
HttpClient http;
http_header_t headers[] = {
  {"Accept", "application/json"},
  {"Content-Type", "application/json"},
  {"api-token", "API-eafee-cdb56-3545d-38f7d"},
  {NULL, NULL}
};
http_request_t request;
http_response_t response;

//ble setup
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

const BleUuid serviceUuid("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid rxUuid("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
const BleUuid txUuid("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

BleCharacteristic txCharacteristic("tx", BleCharacteristicProperty::NOTIFY, txUuid, serviceUuid);
BleCharacteristic rxCharacteristic("rx", BleCharacteristicProperty::WRITE_WO_RSP, rxUuid, serviceUuid, onDataReceived, NULL);

String inputBuffer;
int count, dsid;
bool bleInput = false;
bool ota = false;
int networkCount;
WiFiAccessPoint networks[5];
String networkBuffer;

int time1;
int time2;
int t1, t2, t3, t4;

bool wifiTest = true;
bool timeFix = false;

void setup() {
  EEPROM.get(200, wifiTimeLeft);
  config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(1000 - SLEEP_DELAY);

  Serial.begin(9600);

  //start transmission from accelerometer
  lis.begin(0x18);
  Wire.end();
  lis.begin(I2C_ADDRESS);
  lis.setRange(LIS3DH_RANGE_16_G);
  lis.setDataRate(LIS3DH_DATARATE_100_HZ);

  //pull sdo pin high to reduce power usage, switches i2c address from 0x18 to 0x19
  pinMode(SDO_OUTPUT_PIN, OUTPUT);
  digitalWrite(SDO_OUTPUT_PIN, HIGH);

  //http path to node server which sends data to the api
  request.hostname = "trek.thewcl.com";
  request.port = 3000;
  request.path = "/";

  //turn ble on
  BLE.on();

  BLE.addCharacteristic(txCharacteristic);
  BLE.addCharacteristic(rxCharacteristic);

  BleAdvertisingData data;
  data.appendServiceUUID(serviceUuid);
  BLE.advertise(&data);
  BLE.onConnected(connectCallback);
  BLE.onDisconnected(disconnectCallback);
  
  pinMode(D7, OUTPUT);
  count = 0;

  time1 = millis();
}


void loop() {
  Particle.process();
  if(ota){
    while(!Particle.connected()){
      Particle.process();
      Particle.connect();
    }
    ota = false;
    while(true){Particle.process();}
  }

  time2 = millis();
  if(bleInput | ((time2 - CONFIG_WAIT_TIME >= time1) && WiFi.hasCredentials() && !(BLE.connected()))){
    if(!timeFix){
      Particle.process();
      WiFi.on();
      WiFi.connect();
      while(!WiFi.ready()){}
      Particle.connect();
      Particle.syncTime();
      delay(5000);
      Particle.process();
      Particle.disconnect();
      WiFi.off();
      timeFix = true;
    }

    EEPROM.get(0, dsid);
    EEPROM.get(100, sleepDuration);
    EEPROM.get(200, wifiInterval);
    config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(sleepDuration - SLEEP_DELAY);

    lis.read();
    unixTime = Time.now();
    // isMoving = 0;

    // if(lis.x_g >= 0.8 && lis.x_g <= 1.2){
    //   x = GRAVITY * (lis.x_g - 1);
    //   y = GRAVITY * lis.y_g;
    //   z = GRAVITY * lis.z_g;
    // } else if(lis.y_g >= 0.8 && lis.y_g <= 1.2){
    //   x = GRAVITY * lis.x_g;
    //   y = GRAVITY * (lis.y_g - 1);
    //   z = GRAVITY * lis.z_g;
    // }else if(lis.z_g >= 0.8 && lis.z_g <= 1.2){
    //   x = GRAVITY * lis.x_g;
    //   y = GRAVITY * lis.y_g;
    //   z = GRAVITY * (lis.z_g - 1);
    // }

    Serial.println(lis.x_g);
    Serial.println(lis.y_g);
    Serial.println(lis.z_g);

    // if(abs(x) > 1 || abs(y) > 1 || abs(z) > 1){
    //   isMoving = 1;
    // }

    payload +=  "{\"dsid\":" + String(dsid) + ", \"value\":\"" + String(round(lis.x_g * ROUNDING_FACTOR) / ROUNDING_FACTOR) + "," + String(round(lis.y_g * ROUNDING_FACTOR) / ROUNDING_FACTOR) + "," + String(round(lis.z_g * ROUNDING_FACTOR) / ROUNDING_FACTOR) + "\", \"timestamp\":" + unixTime + "},";
    //payload += String(lis.x_g) + "," + lis.y_g + "," + lis.z_g + "," + unixTime + ",";
    Serial.println(payload);
    Serial.println(dsid);
    Serial.println(sleepDuration);
    Serial.println(wifiInterval);
    Serial.println(t3);

    //lis.setupLowPowerWakeMode(16);

    BLE.disconnect();
    BLE.off();
    System.sleep(config);
    t2 = millis();
    t3 = t2 - t1;
    
    if(wifiTimeLeft <= 0){
      WiFi.on();
      WiFi.connect();
      while(!WiFi.ready()){}
      Particle.connect();
      Particle.syncTime();
      delay(1000);
      Particle.process();
      Serial.println(Particle.timeSyncedLast());
      Serial.println(Time.now());

      payload.remove(payload.length() - 1);
      request.body = "{\"data\":[" + payload + "]}";
      //request.body = dsid + "," + payload;

      http.post(request, response, headers);
      Serial.println("Status: " + response.status);
      Serial.println("Body: " + response.body);

      payload = "";
      
      wifiTimeLeft = wifiInterval;

      Particle.disconnect();
      WiFi.off();
    }

    wifiTimeLeft -= sleepDuration;
  }
}

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context){
  Serial.println(len);
  inputBuffer = "";
  switch(count){
    case 0:{
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
      for(int i = 0; i < len - 1; i++){
        inputBuffer += (char)data[i];
        ssid = inputBuffer;
        Serial.println(data[i]);
      }
      Serial.println(ssid);
      Serial.println(ssid.length());
      if(ssid == ""){
        count = 3;
        EEPROM.get(0, dsid);
        txCharacteristic.setValue("\nCurrent DSID is [");
        if(dsid != -1){
          txCharacteristic.setValue(String(dsid));
        }
        txCharacteristic.setValue("]\nEnter device DSID (blank to skip): ");
      }else if(ssid == "clear"){
        WiFi.clearCredentials();
        count = 0;
        goto SSID;
      }else{
        txCharacteristic.setValue("\nEnter network password: ");
      }
      break;
    }
    case 2:{
      for(int i = 0; i < len - 1; i++){
        Serial.println(data[i]);
        inputBuffer += (char)data[i];
        password = inputBuffer;
      }
      Serial.println(password);
      Serial.println(password.length());
      WiFi.setCredentials(ssid, password);
      Serial.println("\n\nCredentials set with ssid: " + ssid + "\npassword: " + password + "\n\n");
      txCharacteristic.setValue("\nEnter 'test' to test credentials (blank to skip): ");
      break;
    }
    case 3:{
      for(int i = 0; i < len - 1; i++){
        Serial.println(data[i]);
        inputBuffer += (char)data[i];
      }
      if(inputBuffer == "test" && WiFi.hasCredentials()){
        t4 = millis();
        WiFi.on();
        WiFi.connect();
        while(WiFi.connecting() || !WiFi.ready()){
          if(millis() >= t4 + WIFI_TEST_TIMEOUT){
            wifiTest = false;
            Serial.println("timeout");
            Serial.println(millis());
            Serial.println(t4);
            break;
          }
        }
        WiFi.off();
        if(wifiTest){
          txCharacteristic.setValue("Success!\n");
        }else{
          txCharacteristic.setValue("ERROR: WiFi connection timeout\n");
          count = 0;
          wifiTest = true;
          goto SSID;
        }
      }
      EEPROM.get(0, dsid);
      txCharacteristic.setValue("\nCurrent DSID is [");
      if(dsid != -1){
        txCharacteristic.setValue(String(dsid));
      }
      txCharacteristic.setValue("]\nEnter device DSID (blank to skip): ");
      break;
    }
    case 4:{
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
      EEPROM.get(100, sleepDuration);
      txCharacteristic.setValue("\nCurrent value for data collection interval is [");
      if(sleepDuration != -1){
        txCharacteristic.setValue(String(sleepDuration));
      }
      txCharacteristic.setValue("]\nEnter time between data collection as an integer in milliseconds (blank to skip): ");
      break;
    }
    case 5:{
      for(int i = 0; i < len - 1; i++){
        Serial.println(data[i]);
        inputBuffer += (char)data[i];
        sleepDuration = atoi(inputBuffer);
      }
      if(inputBuffer == ""){
        EEPROM.get(100, sleepDuration);
      }
      EEPROM.put(100, sleepDuration);
      EEPROM.get(100, sleepDuration);
      config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(sleepDuration - SLEEP_DELAY);
      Serial.println(sleepDuration);
      EEPROM.get(200, wifiInterval);
      txCharacteristic.setValue("\nCurrent value for WiFi connection interval is [");
      if(wifiInterval != -1){
        txCharacteristic.setValue(String(wifiInterval / 1000));
      }
      txCharacteristic.setValue("]\nEnter time between WiFi connections as an integer in seconds (blank to skip): ");
      break;
    }
    case 6:{
      for(int i = 0; i < len - 1; i++){
        Serial.println(data[i]);
        inputBuffer += (char)data[i];
        wifiInterval = atoi(inputBuffer) * 1000;
      }
      if(inputBuffer == ""){
        EEPROM.get(200, wifiInterval);
      }
      EEPROM.put(200, wifiInterval);
      EEPROM.get(200, wifiInterval);
      wifiTimeLeft = wifiInterval;
      Serial.println(wifiInterval);
      txCharacteristic.setValue("\nEnter 'ota' to wait for OTA update (blank to skip): ");
      break;
    }
    case 7:{
      for(int i = 0; i < len - 1; i++){
        Serial.println(data[i]);
        inputBuffer += (char)data[i];
      }
      if(inputBuffer == "ota"){
        Serial.println("346");
        System.updatesEnabled();
        Serial.println("348");
        ota = true;
      }
      bleInput = true;
      digitalWrite(D7, LOW);
    }
  }

  count++;
}

void connectCallback(const BlePeerDevice& peer, void* context){
  count = 0;
  Serial.println("connected");
  digitalWrite(D7, HIGH);
}

void disconnectCallback(const BlePeerDevice& peer, void* context){
  Serial.println("disconnected");
  digitalWrite(D7, LOW);
}