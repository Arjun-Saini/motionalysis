#include "Adafruit_LIS3DH.h"
#include "MQTT.h"
#include <string>

String payload;
int counter;
const int DELAY = 200; //pause between data readings in milliseconds
const int AWAKE_DURATION = 10000; //how long acceleromter will be awake for in milliseconds
const int CLICK_THRESHHOLD = 15; //higher is less sensitive

Adafruit_LIS3DH lis = Adafruit_LIS3DH();
MQTT client("lab.thewcl.com", 1883, callback);

void setup() {
  Serial.begin(9600);
  //start transmission from accelerometer
  lis.begin(0x18);
  lis.setRange(LIS3DH_RANGE_8_G); //scale of data, 8g makes the axis with gravity on it output 1
  lis.setClick(1, CLICK_THRESHHOLD);

  //connect to MQTT
  client.connect(System.deviceID());
}

void loop() {
  //accelerometer wakes when it starts moving
  if(lis.getClick() > 0){
    counter = AWAKE_DURATION;
  }

  //only reads and outputs data if awake
  if(counter > 0){
    //read data from accelerometer
    lis.read();
    Serial.println(lis.getClick());
    payload = "X: " + String(lis.x_g) + "\tY: " + String(lis.y_g) + "\tZ: " + String(lis.z_g);

    /* not sure why this doesnt work, it should output the acceleration in meters per second squared but instead it crashes the argon with a hard fault
    sensors_event_t event;
    lis.getEvent(&event);
    payload = "X: " + String(event.acceleration.x) + "\tY: " + String(event.acceleration.y) + "\tZ: " + String(event.acceleration.z); 
    */

    //publish to mqtt
    client.publish("test/motionalysis", payload);
  }

  //reconnect to mqtt if necessary
  if(client.isConnected()){
    client.loop();
  } else{
    client.connect(System.deviceID());
  }

  //pause between each loop to slow rate of data gathering
  counter -= DELAY;
  delay(DELAY);
}

void callback(char* topic, byte* payload, unsigned int length){
  
}