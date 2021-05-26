/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "e:/IoT/motionalysis/src/motionalysis.ino"
#include "Adafruit_LIS3DH.h"
#include "MQTT.h"
#include <string>

void setup();
void loop();
void callback(char* topic, byte* payload, unsigned int length);
#line 5 "e:/IoT/motionalysis/src/motionalysis.ino"
String payload;
int counter;
const int DELAY = 200; //pause between data readings in milliseconds
const int AWAKE_DURATION = 20000; //how long accelerometer will be awake for in milliseconds
const int CLICK_THRESHHOLD = 15; //higher is less sensitive

Adafruit_LIS3DH lis = Adafruit_LIS3DH();
MQTT client("lab.thewcl.com", 1883, callback);
SystemSleepConfiguration config;

void setup() {
  Serial.begin(9600);
  //start transmission from accelerometer
  lis.begin(0x18);
  lis.setRange(LIS3DH_RANGE_8_G);
  lis.setClick(1, CLICK_THRESHHOLD);

  //connect to MQTT
  client.connect(System.deviceID());

  pinMode(D5, INPUT);
  config.mode(SystemSleepMode::ULTRA_LOW_POWER).gpio(D5, RISING);
}

void loop() {
  counter -= DELAY;
  Serial.println(counter);
  if(counter <= 0){
    counter = AWAKE_DURATION;
    System.sleep(config);
  }

  //read data from accelerometer
  lis.read();
  payload = "X: " + String(lis.x_g) + "\tY: " + String(lis.y_g) + "\tZ: " + String(lis.z_g);

  /* not sure why this doesnt work, it should output the acceleration in meters per second squared but instead it crashes the argon with a hard fault
  sensors_event_t event;
  lis.getEvent(&event);
  payload = "X: " + String(event.acceleration.x) + "\tY: " + String(event.acceleration.y) + "\tZ: " + String(event.acceleration.z); 
  */

  //publish to mqtt
  client.publish("test/motionalysis", payload);

  //reconnect to mqtt if necessary
  if(client.isConnected()){
    client.loop();
  } else{
    client.connect(System.deviceID());
  }

  //pause between each loop to slow rate of data gathering
  delay(DELAY);
}

void callback(char* topic, byte* payload, unsigned int length){
  
}