#include "HX711.h"  // Make sure you installed HX711 by Bogdan Necula or olkal

#define DOUT  A3
#define CLK   A2

HX711 scale;  // Declare the scale object here

float calibration_factor = -7050; // Adjust later for your load cell

void setup() {
  Serial.begin(9600);
  Serial.println("HX711 Calibration");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press a,s,d,f to increase calibration factor by 10,100,1000,10000 respectively");
  Serial.println("Press z,x,c,v to decrease calibration factor by 10,100,1000,10000 respectively");
  Serial.println("Press t for tare");

  scale.begin(DOUT, CLK);  // Initialize the HX711
  scale.set_scale();       // Reset scale to default
  scale.tare();            // Reset the scale to 0
}

void loop() {
  Serial.print("Reading: ");
  Serial.print(scale.get_units(), 3);
  Serial.print(" kg");
  Serial.print("    calibration_factor: ");
  Serial.println(calibration_factor);

  if (Serial.available()) {
    char temp = Serial.read();
    if (temp == 'a') calibration_factor += 10;
    else if (temp == 's') calibration_factor += 100;
    else if (temp == 'd') calibration_factor += 1000;
    else if (temp == 'f') calibration_factor += 10000;
    else if (temp == 'z') calibration_factor -= 10;
    else if (temp == 'x') calibration_factor -= 100;
    else if (temp == 'c') calibration_factor -= 1000;
    else if (temp == 'v') calibration_factor -= 10000;
    else if (temp == 't') scale.tare();  // Reset scale
    scale.set_scale(calibration_factor);
  }
}
