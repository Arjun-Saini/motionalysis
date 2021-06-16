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
#include <string>

#define I2C_ADDRESS 0x18
#define DELAY 200 //pause between data readings in milliseconds
#define AWAKE_DURATION 5000 //how long argon will be awake for in milliseconds after initial movement
#define CLICK_THRESHHOLD 60 //higher is less sensitive
#define GRAVITY 9.8066
#define INTERRUPT_PIN D5 //pin on argon that is connected to interrupt pin on accelerometer
#define MQTT_DELAY 200 //milliseconds between each publish of mqtt data, can't be too low or else mqtt won't be able to keep up
#define MQTT_PATH "motionalysis/" + System.deviceID()

String xPayload[AWAKE_DURATION / DELAY];
String yPayload[AWAKE_DURATION / DELAY];
String zPayload[AWAKE_DURATION / DELAY];
int timeLeft;
int counter;

Adafruit_LIS3DH lis = Adafruit_LIS3DH();
MQTT client("lab.thewcl.com", 1883, callback);
SystemSleepConfiguration config;

void setup() {
  Serial.begin(9600);

  //start transmission from accelerometer
  lis.begin(I2C_ADDRESS);
  lis.setRange(LIS3DH_RANGE_2_G);
  lis.setClick(1, CLICK_THRESHHOLD);

  pinMode(INTERRUPT_PIN, INPUT);
  config.mode(SystemSleepMode::HIBERNATE).gpio(INTERRUPT_PIN, RISING);

  timeLeft = AWAKE_DURATION + DELAY;
}

void loop() {
  timeLeft -= DELAY;
  if(timeLeft <= 0){
    WiFi.on();
    WiFi.connect();
    while(!WiFi.ready()){}

    //connect and publish to MQTT
    client.connect(System.deviceID());
    for(int i = 0; i < AWAKE_DURATION / DELAY; i++){
      client.publish(MQTT_PATH + "/x", xPayload[i]);
      client.publish(MQTT_PATH + "/y", yPayload[i]);
      client.publish(MQTT_PATH + "/z", zPayload[i]);
      client.loop();
      delay(MQTT_DELAY);
    }

    System.sleep(config);
  }

  //read data from accelerometer
  lis.read();
  xPayload[counter] = String(GRAVITY * lis.x_g);
  yPayload[counter] = String(GRAVITY * lis.y_g);
  zPayload[counter] = String(GRAVITY * lis.z_g);
  counter++;

  //pause between each loop to slow rate of data gathering
  delay(DELAY);
}

void callback(char* topic, byte* payload, unsigned int length){
  
}