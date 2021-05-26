SYSTEM_MODE(MANUAL);

#include "Adafruit_LIS3DH.h"


Adafruit_LIS3DH lis = Adafruit_LIS3DH();

void setup() {
  Serial.begin(9600);
  Wire.begin();
  lis.begin(0x18);
}

void loop() {
  lis.read();
  Serial.print("X:  "); Serial.print(lis.x);
  Serial.print("  \tY:  "); Serial.print(lis.y);
  Serial.print("  \tZ:  "); Serial.print(lis.z);
  Serial.println();
  delay(200);
}
