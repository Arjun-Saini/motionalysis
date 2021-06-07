#include "Adafruit_LIS3DH.h"
#include "MQTT.h"
#include <string>

String payload;
int counter;
const int DELAY = 200; //pause between data readings in milliseconds
const int AWAKE_DURATION = 5000; //how long argon will be awake for in milliseconds after initial movement
const int CLICK_THRESHHOLD = 60; //higher is less sensitive
sensors_event_t event;

Adafruit_LIS3DH lis = Adafruit_LIS3DH();
MQTT client("lab.thewcl.com", 1883, callback);
SystemSleepConfiguration config;

void setup() {
  Serial.begin(9600);

  //start transmission from accelerometer
  lis.begin(0x18);
  lis.setRange(LIS3DH_RANGE_2_G);
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
  payload = "X: " + String(9.8066 * lis.x_g) + "\tY: " + String(9.8066 * lis.y_g) + "\tZ: " + String(9.8066 * lis.z_g);

  //reconnect to mqtt if necessary
  if(client.isConnected()){
    client.loop();
  } else{
    client.connect(System.deviceID());
  }

  //publish to mqtt
  client.publish("test/motionalysis", payload);

  //pause between each loop to slow rate of data gathering
  delay(DELAY);
}

void callback(char* topic, byte* payload, unsigned int length){
  
}