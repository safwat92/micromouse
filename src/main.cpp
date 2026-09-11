#include <Arduino.h>
#include "motors.h"
#include "mpu.h"

void setup()
{
  Serial.begin(115200);

  setupMotors();
  setupEncoders();
  setupMPUDirect();
}

void loop()
{
  readRawMPU();
  printEncoderValue();
}