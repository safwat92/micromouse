#pragma once

struct EncoderData
{
    long leftTicks;
    long rightTicks;
    long avgTicks;  // متوسط الحركة لمركز الماوس
    long diffTicks; // الفرق بين العجلتين لكشف الانحراف
};

void IRAM_ATTR readLeftEncoder();
void IRAM_ATTR readRightEncoder();
EncoderData getEncoderData();
void resetEncoderTicks();
void readEncoder();
void setupMotors();
void setupEncoders();
void printEncoderValues();
void moveMotorsForward(int leftSpeed, int rightSpeed);
void moveMotorsBackward(int leftSpeed, int rightSpeed);
void setMotorSpeeds(int leftSpeed, int rightSpeed);
void stopMotors();
void testMotorsAndEncoders();