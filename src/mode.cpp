#include <Arduino.h>
#include "motors.h"
#include "flood.h"
#include "mpu.h"
#include "tof.h"

#define btn1 34
#define btn2 35

void printCalibrationData()
{
    EncoderData enc = getEncoderData();

    int nextDir = getNextMove();
    int currentDir = get_robot_dir();
    // حساب الفرق بين الاتجاه المطلوب والاتجاه الحالي (0: للأمام, 1: يمين, 2: للدوران للخلف, 3: يسار)
    int turnDiff = (nextDir - currentDir + 4) % 4;

    Serial.println(F("\n================ [ STEP CALIBRATION DATA ] ================"));

    // 1) Robot Position & Heading
    Serial.print(F("Current Cell : ("));
    Serial.print(get_robot_x());
    Serial.print(F(", "));
    Serial.print(get_robot_y());
    Serial.print(F(") | Facing: "));
    if (currentDir == 0)
        Serial.print(F("NORTH (0)"));
    else if (currentDir == 1)
        Serial.print(F("EAST (1)"));
    else if (currentDir == 2)
        Serial.print(F("SOUTH (2)"));
    else if (currentDir == 3)
        Serial.print(F("WEST (3)"));
    Serial.println();

    // 2) IMU Yaw Angle
    Serial.print(F("IMU Yaw Angle: "));
    Serial.print(getYaw());
    Serial.println(F(" deg"));

    // 3) Encoders
    Serial.print(F("Encoders     -> Left: "));
    Serial.print(enc.leftTicks);
    Serial.print(F(" | Right: "));
    Serial.print(enc.rightTicks);
    Serial.print(F(" | Avg: "));
    Serial.println(enc.avgTicks);

    // 4) ToF distances
    Serial.print(F("ToF (mm)     -> LB: "));
    Serial.print(currentTof.leftBack);
    Serial.print(F(" | LF: "));
    Serial.print(currentTof.leftFront);
    Serial.print(F(" | Front: "));
    Serial.print(currentTof.front);
    Serial.print(F(" | RF: "));
    Serial.print(currentTof.rightFront);
    Serial.print(F(" | RB: "));
    Serial.println(currentTof.rightBack);

    // 5) Wall flags
    Serial.print(F("Walls Flags  -> Left: "));
    Serial.print(currentTof.hasLeftWall ? F("[WALL]") : F("[OPEN]"));
    Serial.print(F(" | Front: "));
    Serial.print(currentTof.hasFrontWall ? F("[WALL]") : F("[OPEN]"));
    Serial.print(F(" | Right: "));
    Serial.println(currentTof.hasRightWall ? F("[WALL]") : F("[OPEN]"));

    // 6) Next move decision
    Serial.print(F("Next Move    -> Turn: "));
    if (turnDiff == 0)
    {
        Serial.print(F("FORWARD (NONE)"));
    }
    else if (turnDiff == 1)
    {
        Serial.print(F("RIGHT"));
    }
    else if (turnDiff == 2)
    {
        Serial.print(F("UTURN (BACK)"));
    }
    else if (turnDiff == 3)
    {
        Serial.print(F("LEFT"));
    }
    Serial.print(F(" to target dir: "));
    Serial.println(nextDir);

    // 7) Maze Map
    printMaze(get_robot_x(), get_robot_y(), get_robot_dir());
}

void success_led(bool value)
{
    digitalWrite(2, value);
}