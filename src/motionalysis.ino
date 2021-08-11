SYSTEM_MODE(MANUAL)
PRODUCT_ID(15161)
PRODUCT_VERSION(1)

#include "Adafruit_LIS3DH.h"
#include "HttpClient/HttpClient.h"
#include <string>

#define I2C_ADDRESS 0x19 //i2c address for lis3dh accelerometer
#define SDO_OUTPUT_PIN D8 //reduces power usage on lis3dh
#define CONFIG_WAIT_TIME 30000 //time in ms after power up where device waits for user input, if none is detected it will start collecting data
#define DEFAULT_SLEEP_DURATION 1000
#define DEFAULT_WIFI_INTERVAL 60000
#define SLEEP_DELAY 70
#define WIFI_TEST_TIMEOUT 30000 //time in ms after wifi test before timing out
#define SENSITIVITY_TOLERANCE 0.5 //minimum change in g value for new data to be sent

int sleepDuration = DEFAULT_SLEEP_DURATION;
int wifiInterval = DEFAULT_WIFI_INTERVAL;

String payload, prevPayload = "";
bool valuesChanged = false;
int wifiTimeLeft;
String unixTime;
String ssid, password = "";

Vector<float> previousArr(3 * (3600 + 1));

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
int count, dsid, size;
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
  System.updatesEnabled();
  EEPROM.get(200, wifiTimeLeft);

  //sets up default config for sleep
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

  //connects to particle cloud if ota command entered through ble
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
    //connects to particle cloud once to synchronize the time
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

    //get values stored in eeprom
    EEPROM.get(0, dsid);
    EEPROM.get(100, sleepDuration);
    EEPROM.get(200, wifiInterval);
    config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(sleepDuration - SLEEP_DELAY);

    //get data from accelerometer
    lis.read();
    unixTime = Time.now();
    
    Serial.println(lis.x_g);
    Serial.println(lis.y_g);
    Serial.println(lis.z_g);
    
    //determines if the new data has changed enough for different values to be sent
    if(abs(lis.x_g - previousArr.at(size)) > SENSITIVITY_TOLERANCE || abs(lis.y_g - previousArr.at(size + 1)) > SENSITIVITY_TOLERANCE || abs(lis.z_g - previousArr.at(size + 2)) > SENSITIVITY_TOLERANCE){
      valuesChanged = true;
      previousArr.append(lis.x_g);
      previousArr.append(lis.y_g);
      previousArr.append(lis.z_g);
      
    }
    
    Serial.println("VALUES");
    Serial.println(previousArr.at(size));
    Serial.println(previousArr.at(size + 1));
    Serial.println(previousArr.at(size + 2));
    prevPayload += "{\"dsid\":" + String(dsid) + ", \"value\":\"" + String(previousArr.at(size)) + "," + String(previousArr.at(size + 1)) + "," + String(previousArr.at(size + 2)) + "\", \"timestamp\":" + unixTime + "},";
    payload += "{\"dsid\":" + String(dsid) + ", \"value\":\"" + String(lis.x_g) + "," + String(lis.y_g) + "," + String(lis.z_g) + "\", \"timestamp\":" + unixTime + "},";

    size += 3;

    Serial.println(payload);
    Serial.println(prevPayload);
    Serial.println(dsid);
    Serial.println(sleepDuration);
    Serial.println(wifiInterval);
    Serial.println(t3);

    //automatically disconnects from ble if it is still connected before sleep, this step is necessary or else it causes the device to crash
    BLE.disconnect();
    BLE.off();

    System.sleep(config);
    t2 = millis();
    t3 = t2 - t1;
    
    //connects to wifi and publishes data
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

      //formatting payload into json format, sets the correct payload as the http request body
      payload.remove(payload.length() - 1);
      prevPayload.remove(prevPayload.length() - 1);
      if(valuesChanged){
        request.body = "{\"data\":[" + payload + "]}";
        valuesChanged = false;
        Serial.println("Values changed, different array sent");
      }else{
        request.body = "{\"data\":[" + prevPayload + "]}";
        Serial.println("Values not changed, same array sent");
      }

      //sends post request to server
      http.post(request, response, headers);
      Serial.println("Status: " + response.status);
      Serial.println("Body: " + response.body);

      //reset variables
      payload = "";
      prevPayload = "";
      size = 0;
      previousArr.clear();
      
      wifiTimeLeft = wifiInterval;

      Particle.disconnect();
      WiFi.off();
    }

    //countdown to wifi publish
    wifiTimeLeft -= sleepDuration;
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
      if(inputBuffer == "test" && WiFi.hasCredentials()){
        //attempt to connect with credentials, times out if unsuccessful
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
          //go back to ssid prompt if test failed
          txCharacteristic.setValue("ERROR: WiFi connection timeout\n");
          count = 0;
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

      //prompt for data collection interval
      txCharacteristic.setValue("\nCurrent value for data collection interval is [");
      if(sleepDuration != -1){
        txCharacteristic.setValue(String(sleepDuration));
      }
      txCharacteristic.setValue("]\nEnter time between data collection as an integer in milliseconds (blank to skip): ");
      break;
    }
    case 5:{
      //store data collection interval in eeprom
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

      //prompt for wifi connection interval
      txCharacteristic.setValue("\nCurrent value for WiFi connection interval is [");
      if(wifiInterval != -1){
        txCharacteristic.setValue(String(wifiInterval / 1000));
      }
      txCharacteristic.setValue("]\nEnter time between WiFi connections as an integer in seconds (blank to skip): ");
      break;
    }
    case 6:{
      //store wifi connection interval in eeprom
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
      bleInput = true;
      digitalWrite(D7, LOW);
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