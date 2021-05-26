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
Adafruit_LIS3DH lis = Adafruit_LIS3DH();
MQTT client("lab.thewcl.com", 1883, callback);

String payload;

void setup() {
  //start transmission from accelerometer
  lis.begin(0x18);

  //connect to MQTT
  client.connect(System.deviceID());
}

void loop() {
  //read data from acceleromter
  lis.read();
  payload = "X: " + String(lis.x_g) + "\tY: " + String(lis.y_g) + "\tZ: " + String(lis.z_g);

  //sensors_event_t event;
  //lis.getEvent(&event);

  //x = event.acceleration.x;
  //y = event.acceleration.y;
  //z = event.acceleration.z;

  //publish to mqtt
  client.publish("test/motionalysis", payload);

  //reconnect to mqtt if necessary
  if(client.isConnected()){
    client.loop();
  } else{
    client.connect(System.deviceID());
  }

  delay(1000);
}

void callback(char* topic, byte* payload, unsigned int length){
  
}