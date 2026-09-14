#include <Arduino.h>
#include "motors.h"
#include "mpu.h"
#include "tof.h"
#include "flood.h"
#include "pid.h"

const int square_ticks = 1000;
const float WALL_THRESHOLD = 130.0;

bool new_wall_found = false;
bool arrived_center = false;

void setup()
{
  Serial.begin(115200);

  setupMotors();
  setupEncoders();

  // setup wire
  setupTof();
  // setupMPUDirect();

  initMaze();
}

void loop()
{
  // readRawMPU();
  // printEncoderValue();
  // readTof();

  // 1) read sensors here (encoders & tof & imu)
  // 2) calculating the maze (flood fill)
  if (new_wall_found) {
    flood(true);
    new_wall_found = false;
  }
  // don't call each iteration, only when tof find new wall
  // 3) get the next move (-1 condition)
  int nextMove = getNextMove();
  if (nextMove == -1)
    return;
  // 4) calculate the motors power (pid -> forward & steer)
  int left_pwm = 0, right_pwm = 0;
  pid(0, 0, 0, 0, left_pwm, right_pwm);
  // 4) output the power to the motors to move

  // ### after passing first cell we check:
  if (ticks >= square_ticks)
  {
    ticks = 0; // reset the encoder ticks for the next cell
    // 5) if walls found (tof) update the walls in memory & recalculate maze
    bool hasLeftWall = (tof_left < WALL_THRESHOLD);
    bool hasFrontWall = (tof_front < WALL_THRESHOLD);
    bool hasRightWall = (tof_right < WALL_THRESHOLD);

    if (hasLeftWall || hasFrontWall || hasRightWall)
    {
      setWall(get_robot_x(), get_robot_y(), get_robot_dir());
      new_wall_found = true;
    };
    // 6) if arrived to the center -> explore to start
    if (isCenter(get_robot_x(), get_robot_y())) {
      arrived_center = true;
    }
    // 7) if the last exploration hasn't explored new walls -> fast runnn!
    // 8) use pre calculated path to move as fast as possible
  }

  // optional:
  // - smooth diagonal algorithms
  // - don't stop the motors each cell in fast run (pid)
  // - freeRTOS

  // potintial bugs
  // - calculate dt using macros
  // - integral windup in pid (reset after each cell)
  // - add imu in case lost tof reading
}