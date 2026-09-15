#include <Arduino.h>
#include "motors.h"

#define MOTOR_L_IN1 19
#define MOTOR_L_IN2 18
#define ENC_L_A 14
#define ENC_L_B 27

#define MOTOR_R_IN1 32
#define MOTOR_R_IN2 33
#define ENC_R_A 26
#define ENC_R_B 25

volatile long leftEncoderTicks = 0;
volatile long rightEncoderTicks = 0;

unsigned long lastPrintTime = 0;
unsigned long stateTimer = 0;
bool isForward = true;

void IRAM_ATTR readLeftEncoder()
{
    int bState = digitalRead(ENC_L_B);
    if (bState == HIGH)
        leftEncoderTicks--;
    else
        leftEncoderTicks++;
}

void IRAM_ATTR readRightEncoder()
{
    int bState = digitalRead(ENC_R_B);
    if (bState == HIGH)
        rightEncoderTicks++;
    else
        rightEncoderTicks--;
}

void setupMotors()
{
    pinMode(MOTOR_L_IN1, OUTPUT);
    pinMode(MOTOR_L_IN2, OUTPUT);
    pinMode(MOTOR_R_IN1, OUTPUT);
    pinMode(MOTOR_R_IN2, OUTPUT);
    stateTimer = millis();
}

void setupEncoders()
{
    pinMode(ENC_L_A, INPUT_PULLUP);
    pinMode(ENC_L_B, INPUT_PULLUP);
    pinMode(ENC_R_A, INPUT_PULLUP);
    pinMode(ENC_R_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(ENC_L_A), readLeftEncoder, RISING);
    attachInterrupt(digitalPinToInterrupt(ENC_R_A), readRightEncoder, RISING);
}

EncoderData getEncoderData()
{
    EncoderData data;

    noInterrupts();
    data.leftTicks = leftEncoderTicks;
    data.rightTicks = rightEncoderTicks;
    interrupts();

    data.avgTicks = (data.leftTicks + data.rightTicks) / 2;
    data.diffTicks = data.leftTicks - data.rightTicks;

    return data;
}

void resetEncoderTicks()
{
    noInterrupts();
    leftEncoderTicks = 0;
    rightEncoderTicks = 0;
    interrupts();
}

void printEncoderValues()
{
    if (millis() - lastPrintTime >= 150)
    {
        lastPrintTime = millis();

        EncoderData enc = getEncoderData();

        Serial.print("Left: ");
        Serial.print(enc.leftTicks);
        Serial.print(" | Right: ");
        Serial.print(enc.rightTicks);
        Serial.print(" | Avg (Center): ");
        Serial.print(enc.avgTicks);
        Serial.print(" | Diff: ");
        Serial.println(enc.diffTicks);
    }
}

void moveMotorsForward(int leftSpeed, int rightSpeed)
{
    setMotorSpeeds(leftSpeed, rightSpeed);
}

void moveMotorsBackward(int leftSpeed, int rightSpeed)
{
    setMotorSpeeds(-leftSpeed, -rightSpeed);
}

void setMotorSpeeds(int leftSpeed, int rightSpeed)
{
    leftSpeed = constrain(leftSpeed, -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);

    if (leftSpeed >= 0)
    {
        analogWrite(MOTOR_L_IN1, leftSpeed);
        analogWrite(MOTOR_L_IN2, 0);
    }
    else
    {
        analogWrite(MOTOR_L_IN1, 0);
        analogWrite(MOTOR_L_IN2, -leftSpeed);
    }

    if (rightSpeed >= 0)
    {
        analogWrite(MOTOR_R_IN1, rightSpeed);
        analogWrite(MOTOR_R_IN2, 0);
    }
    else
    {
        analogWrite(MOTOR_R_IN1, 0);
        analogWrite(MOTOR_R_IN2, -rightSpeed);
    }
}

void stopMotors()
{
    analogWrite(MOTOR_L_IN1, 0);
    analogWrite(MOTOR_L_IN2, 0);
    analogWrite(MOTOR_R_IN1, 0);
    analogWrite(MOTOR_R_IN2, 0);
}

void testMotorsAndEncoders()
{
    if (millis() - stateTimer >= 2000)
    {
        stateTimer = millis();
        isForward = !isForward;

        if (isForward)
        {
            moveMotorsForward(200, 200);
        }
        else
        {
            moveMotorsBackward(200, 200);
        }
    }
}

// void setup()
// {
//     Serial.begin(115200);
//     setupMotors();
//     setupEncoders();

//     // انطلاق مبدئي للأمام للاختبار
//     moveMotorsForward(200, 200);
// }

// void loop()
// {
//     printEncoderValues();
//     testMotorsAndEncoders();
// }