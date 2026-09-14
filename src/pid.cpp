#include <Arduino.h>

float fwd_kp = 0.1, fwd_ki = .01, fwd_kd = 0.05;
float steer_kp = 0.1, steer_ki = 0, steer_kd = 0.05;
float fwd_integral = 0;
float fwd_prev_error = 0;
float steer_integral = 0;
float steer_prev_error = 0;

void pid(float curr_ticks
     , float target_ticks, float tof_left,
      float tof_right, int &left_pwm, int &right_pwm) { // curr_ticks -> average of 2 encoders
    float fwd_error = target_ticks - curr_ticks;

    fwd_integral += fwd_error;

    float fwd_derivative = (fwd_error - fwd_prev_error);

    fwd_prev_error = fwd_error;

    float fwd_power = (fwd_kp * fwd_error) + (fwd_ki * fwd_integral) + (fwd_kd * fwd_derivative);

    float steer_power = 0;
    if (tof_left < 150 && tof_right < 150)
    {                                            
        float steer_error = tof_left - tof_right;

        steer_integral += steer_error;

        float steer_derivative = (steer_error - steer_prev_error);

        steer_prev_error = steer_error;

        steer_power = (steer_kp * steer_error) + (steer_ki * steer_integral) + (steer_kd * steer_derivative);
    }

    left_pwm = constrain(fwd_power - steer_power, -255, 255);
    right_pwm = constrain(fwd_power + steer_power, -255, 255);
}

// void driveForwardPID(int target_mm, int base_speed)
// {
//     left_ticks = 0;
//     right_ticks = 0;

//     while ((left_ticks + right_ticks) / 2 < mmToTicks(target_mm))
//     {
//         int left_dist = readToF(3);  // Left Sensor
//         int right_dist = readToF(4); // Right Sensor

//         // حساب الخطأ للمحافظة على المنتصف
//         if (left_dist < 120 && right_dist < 120)
//         {
//             error = left_dist - right_dist;
//         }
//         else if (left_dist < 120)
//         {
//             error = (left_dist - 60) * 2;
//         }
//         else if (right_dist < 120)
//         {
//             error = (60 - right_dist) * 2;
//         }
//         else
//         {
//             error = left_ticks - right_ticks; // الاعتماد على الـ Encoders في الممرات المفتوحة
//         }

//         integral += error;
//         float derivative = error - last_error;
//         float correction = (Kp * error) + (Ki * integral) + (Kd * derivative);
//         last_error = error;

//         setMotorSpeeds(base_speed - correction, base_speed + correction);
//         delay(10);
//     }
//     setMotorSpeeds(0, 0);
// }

// float curr = 0;
// float target = 100;
// ######### loop ########
// if (curr > target + 100)
//     return;

// float motor_power = pid(curr, target);

// Serial.print("Current Position: ");
// Serial.print(curr);
// Serial.print(" | Motor Power: ");
// Serial.println(motor_power);

// curr += 10;