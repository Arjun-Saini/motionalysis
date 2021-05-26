#include "Adafruit_LIS3DH.h"
#include "MQTT.h"
#include <string>

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

  //pause between each loop to slow rate of data gathering
  delay(1000);
}

void callback(char* topic, byte* payload, unsigned int length){
  
}