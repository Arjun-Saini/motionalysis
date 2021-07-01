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
#define WIFI_INTERVAL 10000; //delay between each publish to mqtt in milliseconds

String payload = "";
int wifiTimeLeft = WIFI_INTERVAL;

Adafruit_LIS3DH lis = Adafruit_LIS3DH();
//MQTT client("lab.thewcl.com", 1883, callback);
SystemSleepConfiguration config;

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
  lis.begin(I2C_ADDRESS);
  lis.setRange(LIS3DH_RANGE_2_G);
  lis.setDataRate(LIS3DH_DATARATE_400_HZ);

  //setup sleep system with interrupt pin
  pinMode(SDO_OUTPUT_PIN, OUTPUT);
  digitalWrite(SDO_OUTPUT_PIN, HIGH);

  config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(SLEEP_DURATION);

  request.hostname = "ptsv2.com";
  request.port = 80;
  request.path = "/t/q2wns-1625165230/post";

  //request.hostname = "api.getshiftworx.com";
  //request.path = "v1/datasource/data";

  wifiTimeLeft = WIFI_INTERVAL;
}

void loop() {
  lis.read();
  //payload += "{\"x\":\"" + String(GRAVITY * lis.x_g) + "\"," + "\"y\":\"" + String(GRAVITY * lis.y_g) + "\"," + "\"z\":\"" + String(GRAVITY * lis.z_g) + "\"},";
  //payload +=  "{\"dsid\":50983, \"value\":3},";

  lis.setupLowPowerWakeMode(16);
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
    //payload.remove(payload.length() - 1);
    //request.body = "{\"data\":[" + payload + "]}";
    request.body = "{\"data\":[{\"dsid\":50983,\"value\":5}]}";
    http.post(request, response, headers);
    delay(1000);
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