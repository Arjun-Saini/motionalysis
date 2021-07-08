/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/Arjun/Documents/GitHub/motionalysis/src/motionalysis.ino"
void setup();
void loop();
void callback(char* topic, byte* payload, unsigned int length);
#line 1 "c:/Users/Arjun/Documents/GitHub/motionalysis/src/motionalysis.ino"
SYSTEM_MODE(MANUAL)

#include "Adafruit_LIS3DH.h"
#include "MQTT.h"
#include "HttpClient/HttpClient.h"
#include <string>

#define I2C_ADDRESS 0x19
#define SLEEP_DURATION 1000 //how long argon will be awake for in milliseconds after initial movement
#define GRAVITY 9.8066
#define MQTT_PATH "motionalysis/" + System.deviceID() //each argon will have its own path in mqtt, with separate paths for each axis of movement
#define SDO_OUTPUT_PIN D8
#define WIFI_INTERVAL 20000; //delay between each publish to mqtt in milliseconds

String payload = "";
int wifiTimeLeft = WIFI_INTERVAL;
float x, y, z;
String unixTime;
int isMoving;

Adafruit_LIS3DH lis = Adafruit_LIS3DH();
//MQTT client("lab.thewcl.com", 1883, callback);
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

void setup() {
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

  config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(SLEEP_DURATION);

  //test post request server, this works correctly
  request.hostname = "trek.thewcl.com";
  request.port = 3000;
  request.path = "/";

  wifiTimeLeft = WIFI_INTERVAL;
}

void loop() {
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

  if(abs(x) > 1 || abs(y) > 1 || abs(z) > 1){
    isMoving = 1;
  }

  payload +=  "{\"dsid\":50983, \"value\":" + String(isMoving) + ", \"timestamp\":" + unixTime + "},";

  //lis.setupLowPowerWakeMode(16);
  System.sleep(config);
  
  if(wifiTimeLeft <= 0){
    WiFi.on();
    WiFi.connect();
    while(!WiFi.ready()){}
    Particle.connect();

    //connect and publish to MQTT
    /*Serial.println(payload);
    while(!client.isConnected()){
      client.connect(System.deviceID());
    
    client.publish(MQTT_PATH, "[" + payload + "]");
    client.loop();*/
    payload.remove(payload.length() - 1);
    request.body = "{\"data\":[" + payload + "]}";

    //request.body = "{\"data\":[{\"dsid\":50983,\"value\":0}]}";
    http.post(request, response, headers);
    Serial.println("Status: " + response.status);
    Serial.println("Body: " + response.body);

    payload = "";
    
    wifiTimeLeft = WIFI_INTERVAL;

    Particle.disconnect();
    WiFi.off();
  }

  wifiTimeLeft -= SLEEP_DURATION;
}

void callback(char* topic, byte* payload, unsigned int length){
  
}