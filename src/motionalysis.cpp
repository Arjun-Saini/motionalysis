/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "e:/IoT/motionalysis/src/motionalysis.ino"
void setup();
void loop();
#line 1 "e:/IoT/motionalysis/src/motionalysis.ino"
SYSTEM_MODE(MANUAL);

#include "Adafruit_LIS3DH.h"
#include "Adafruit_Sensor.h"

Adafruit_LIS3DH lis = Adafruit_LIS3DH();

void setup() {
  Wire.begin();
  Serial.begin(9600);
  lis.begin(0x18);
}

sensors_event_t event1;

void loop() {
  lis.read();      // get X Y and Z data at once
  // Then print out the raw data
  Serial.print("X:  "); Serial.print(lis.x_g);
  Serial.print("  \tY:  "); Serial.print(lis.y_g);
  Serial.print("  \tZ:  "); Serial.print(lis.z_g);

  /* Or....get a new sensor event, normalized */
  //sensors_event_t event;
  //lis.getEvent(&event);

  /* Display the results (acceleration is measured in m/s^2) */
  //Serial.print("\t\tX: "); Serial.print(event.acceleration.x);
  //Serial.print(" \tY: "); Serial.print(event.acceleration.y);
  //Serial.print(" \tZ: "); Serial.print(event.acceleration.z);
  //Serial.println(" m/s^2 ");

  Serial.println();

  delay(200);
}
