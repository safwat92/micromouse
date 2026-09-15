#pragma once

void pid(float curr_ticks, float target_ticks, float tof_left,
         float tof_right, int &left_pwm, int &right_pwm);

void resetPID();
void driveOneCell(int baseSpeed = 140, int targetTicks = 1000);