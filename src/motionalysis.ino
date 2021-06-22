SYSTEM_MODE(MANUAL)

#include "Adafruit_LIS3DH.h"
#include "MQTT.h"
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
MQTT client("lab.thewcl.com", 1883, callback);
SystemSleepConfiguration config;

void setup() {
  Serial.begin(9600);

  //start transmission from accelerometer
  lis.begin(I2C_ADDRESS);
  lis.setRange(LIS3DH_RANGE_2_G);

  //setup sleep system with interrupt pin
  pinMode(SDO_OUTPUT_PIN, OUTPUT);
  digitalWrite(SDO_OUTPUT_PIN, HIGH);

  config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(SLEEP_DURATION);

  wifiTimeLeft = WIFI_INTERVAL;
}

void loop() {
  lis.read();
  Serial.println(lis.z_g);
  payload += "{\"x\":\"" + String(GRAVITY * lis.x_g) + "\"," + "\"y\":\"" + String(GRAVITY * lis.y_g) + "\"," + "\"z\":\"" + String(GRAVITY * lis.z_g) + "\"},";
  System.sleep(config);
  
  if(wifiTimeLeft <= 0){
    WiFi.on();
    WiFi.connect();
    while(!WiFi.ready()){}

    //connect and publish to MQTT
    client.connect(System.deviceID());
    client.publish(MQTT_PATH, "[" + payload + "]");
    client.loop();

    payload = "";
    wifiTimeLeft = WIFI_INTERVAL;
  }

  wifiTimeLeft -= SLEEP_DURATION;
}

void callback(char* topic, byte* payload, unsigned int length){
  
}