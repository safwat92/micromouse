#include <Arduino.h>
#define in1 16
#define in2 17
#define encoder1 18
#define encoder2 19

volatile int encoderTicks = 0;
unsigned long lastPrintTime = 0;
unsigned long stateTimer = 0;
bool isForward = true;

void readEncoder()
{
    int bState = digitalRead(encoder2);
    if (bState == HIGH)
    {
        encoderTicks++;
    }
    else
    {
        encoderTicks--;
    }
}

void setupMotors()
{
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    stateTimer = millis();
}

void setupEncoders()
{
    pinMode(encoder1, INPUT_PULLUP);
    pinMode(encoder2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(encoder1), readEncoder, RISING);
}

void printEncoderValue()
{
    if (millis() - lastPrintTime >= 150)
    {
        lastPrintTime = millis();

        Serial.print("Encoder Ticks: ");
        Serial.print(encoderTicks);
    }
}

void moveMotorForward()
{
    analogWrite(in1, 255);
    analogWrite(in2, 0);
}

void moveMotorBackward()
{
    analogWrite(in1, 0);
    analogWrite(in2, 255);
}

void testMotors()
{
    if (millis() - stateTimer >= 2000)
    {
        stateTimer = millis();
        isForward = !isForward;

        if (isForward)
        {
            moveMotorForward();
        }
        else
        {
            moveMotorBackward();
        }
    }
}