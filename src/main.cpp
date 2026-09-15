#include <Arduino.h>
#include <Wire.h>
#include "motors.h"
#include "mpu.h"
#include "tof.h"
#include "flood.h"
#include "pid.h"
#include "mode.h"
#include "i2c_diag.h"

const bool RUN_I2C_DIAGNOSTIC_ONLY = false;

const int square_ticks = 620;
const int BASE_SPEED = 70;
const unsigned long STEP_PAUSE_MS = 2000;

bool arrived_center = false;
bool exploring_to_center = true;

void setup()
{
  Serial.begin(115200);

  Serial.println(F("\n\n>>> Starting Micromouse Boot Sequence <<<"));

  Wire.begin(21, 22);
  Wire.setClock(100000);
  Wire.setTimeOut(50);

  pinMode(2, OUTPUT);

  setupMotors();
  setupEncoders();

  setupMPUDirect();
  setupTof();

  // فحص وتشخيص سريع لناقل الـ I2C للكشف عن أي حساس لا يستجيب
  scanAndDiagnoseI2C();

  if (RUN_I2C_DIAGNOSTIC_ONLY)
  {
    Serial.println(F("\n>>> I2C DIAGNOSTIC MODE ACTIVE <<<"));
    Serial.println(F("Robot will NOT move. Testing sensors continuously...\n"));
    return;
  }

  initMaze();

  // Initial floodfill towards center
  flood(true);

  success_led(true);
  Serial.println(F("Micromouse Initialized and Ready!"));
}

void loop()
{
  if (RUN_I2C_DIAGNOSTIC_ONLY)
  {
    runFullI2CHardwareTest();
    delay(3000);
    return;
  }
  // 1) Read sensors (ToF then IMU — ToF is slow, so read IMU last for accurate dt)
  updateTofData();
  updateYaw();

  // 2) Detect and record walls in memory
  bool new_wall = updateWalls(currentTof.hasFrontWall, currentTof.hasRightWall, currentTof.hasLeftWall);

  // 3) Recalculate floodfill if a new wall was discovered
  if (new_wall)
  {
    flood(exploring_to_center);
  }

  // 4) Check if target destination is reached
  if (exploring_to_center && isCenter(get_robot_x(), get_robot_y()))
  {
    Serial.println(F("\n>>> Arrived at Center! <<<"));
    printCalibrationData();
    stopMotors();
    arrived_center = true;
    exploring_to_center = false;
    delay(3000);
    // Recalculate floodfill to return to start (0, 0)
    flood(false);
    return;
  }
  else if (!exploring_to_center && get_robot_x() == 0 && get_robot_y() == 0)
  {
    Serial.println(F("\n>>> Returned to Start! Exploration Complete! <<<"));
    printCalibrationData();
    stopMotors();
    while (true)
    {
      delay(1000);
    }
  }

  // 5) Get next optimal direction from FloodFill
  int nextDir = getNextMove();
  if (nextDir == -1)
  {
    Serial.println(F("\n[!] Error: No reachable path!"));
    printCalibrationData();
    stopMotors();
    while (true)
    {
      delay(1000);
    }
  }

  // 6) Print complete calibration & sensor data at this cell
  printCalibrationData();

  // 7) Pause to observe the robot and review serial monitor data
  Serial.println(F("Pausing at cell before next move..."));
  delay(STEP_PAUSE_MS);

  // 8) Rotate robot to face target direction using IMU
  if (nextDir != get_robot_dir())
  {
    turnToDirection(nextDir);
    delay(200);
  }

  // 9) Drive forward one cell slowly using PID
  driveOneCell(BASE_SPEED, square_ticks);

  // 10) Update coordinates of robot in the maze
  advanceRobotCoordinates();

  // Settle at cell
  stopMotors();
  delay(300);
}