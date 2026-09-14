#pragma once

void pid(float curr_ticks, float target_ticks, float tof_left,
         float tof_right, int &left_pwm, int &right_pwm);