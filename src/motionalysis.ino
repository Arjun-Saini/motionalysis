SYSTEM_MODE(MANUAL)

#include "Adafruit_LIS3DH.h"
#include "MQTT.h"
#include "HttpClient/HttpClient.h"
#include <string>

#define I2C_ADDRESS 0x19
#define GRAVITY 9.8066
#define SDO_OUTPUT_PIN D8

int sleepDuration = 1000;
int wifiInterval = 5000;

String payload = "";
int wifiTimeLeft;
float x, y, z;
String unixTime;
int isMoving;
String ssid, password;

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

int count, dsid;
bool bleInput = false;

void setup() {
  wifiTimeLeft = wifiInterval;
  config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(1000);

  Serial.begin(9600);

  //start transmission from accelerometer
  lis.begin(0x18);
  Wire.end();
  lis.begin(I2C_ADDRESS);
  lis.setRange(LIS3DH_RANGE_2_G);
  lis.setDataRate(LIS3DH_DATARATE_400_HZ);

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
}


void loop() {  
  if(bleInput){
    lis.read();
    unixTime = Time.now();
    isMoving = 0;

    if(lis.x_g >= 0.8 && lis.x_g <= 1.2){
      x = GRAVITY * (lis.x_g - 1);
      y = GRAVITY * lis.y_g;
      z = GRAVITY * lis.z_g;
    } else if(lis.y_g >= 0.8 && lis.y_g <= 1.2){
      x = GRAVITY * lis.x_g;
      y = GRAVITY * (lis.y_g - 1);
      z = GRAVITY * lis.z_g;
    }else if(lis.z_g >= 0.8 && lis.z_g <= 1.2){
      x = GRAVITY * lis.x_g;
      y = GRAVITY * lis.y_g;
      z = GRAVITY * (lis.z_g - 1);
    }

    Serial.println(lis.x_g);
    Serial.println(lis.y_g);
    Serial.println(lis.z_g);

    if(abs(x) > 1 || abs(y) > 1 || abs(z) > 1){
      isMoving = 1;
    }

    payload +=  "{\"dsid\":" + String(dsid) + ", \"value\":" + String(isMoving) + ", \"timestamp\":" + unixTime + "},";
    Serial.println(payload);

    //lis.setupLowPowerWakeMode(16);

    BLE.disconnect();
    BLE.off();
    System.sleep(config);
    
    if(wifiTimeLeft <= 0){
      WiFi.on();
      WiFi.connect();
      while(!WiFi.ready()){}
      Particle.connect();

      payload.remove(payload.length() - 1);
      request.body = "{\"data\":[" + payload + "]}";

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
  if(count == 0){
    txCharacteristic.setValue("Enter network SSID (0 to skip): ");
  }else if(count == 1){
    ssid = (char *)data;
    Serial.println(ssid);
    if(ssid == "0"){
      count = 2;
      txCharacteristic.setValue("Enter device DSID: ");
    }else{
      txCharacteristic.setValue("Enter network password: ");
    }
  }else if(count == 2){
    password = (char *)data;
    WiFi.setCredentials(ssid, password);
    Serial.println("Credentials set with ssid: " + ssid + "\n\tpassword: " + password);
    txCharacteristic.setValue("Enter device DSID: ");
  }else if(count == 3){
    if((String)(char *)data != ""){
      EEPROM.put(0, atoi((char *)data));
    }
    EEPROM.get(0, dsid);
    Serial.println(dsid);
    txCharacteristic.setValue("Enter time between data collection: ");
  }else if(count == 4){
    sleepDuration = atoi((char *)data);
    config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(sleepDuration);
    Serial.println(sleepDuration);
    txCharacteristic.setValue("Enter time between WiFi connection: ");
  }else if(count == 5){
    wifiInterval = atoi((char *)data);
    Serial.println(wifiInterval);
    bleInput = true;
    digitalWrite(D7, LOW);
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