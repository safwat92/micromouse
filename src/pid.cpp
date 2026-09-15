#include <Arduino.h>
#include "pid.h"
#include "motors.h"
#include "mpu.h"
#include "tof.h"

float fwd_kp = 0.1, fwd_ki = 0.01, fwd_kd = 0.05;
float steer_kp = 0.4, steer_ki = 0.001, steer_kd = 0.1;
float fwd_integral = 0;
float fwd_prev_error = 0;
float steer_integral = 0;
float steer_prev_error = 0;

void resetPID()
{
    fwd_integral = 0;
    fwd_prev_error = 0;
    steer_integral = 0;
    steer_prev_error = 0;
}

void pid(float curr_ticks, float target_ticks, float tof_left,
         float tof_right, int &left_pwm, int &right_pwm)
{
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

void driveOneCell(int baseSpeed, int targetTicks)
{
    resetEncoderTicks();

    float straightYaw = getYaw();
    const float IDEAL_SIDE_DIST = 55.0f;
    const float WALL_VALID_DIST = 130.0f;
    const float FRONT_WALL_STOP = 85.0f;
    const float DECEL_ZONE = targetTicks * 0.3f;
    const float MIN_SPEED_FACTOR = 0.4f;

    // Local PID variables — عشان ما تتأثرش بأي دالة تانية
    float local_steer_integral = 0;
    float local_steer_prev_error = 0;

    unsigned long startTime = millis();
    const unsigned long DRIVE_TIMEOUT = 5000; // 5 ثوان كحد أقصى

    while (true)
    {
        updateYaw();
        updateTofSteering();

        EncoderData enc = getEncoderData();

        // 1. Timeout safety
        if (millis() - startTime > DRIVE_TIMEOUT)
        {
            Serial.println(F("WARNING: driveOneCell timeout!"));
            break;
        }

        // 2. Target distance reached
        if (enc.avgTicks >= targetTicks)
        {
            break;
        }

        // 3. Safety stop: front wall detected dangerously close
        if (currentTof.hasFrontWall && currentTof.front < FRONT_WALL_STOP)
        {
            break;
        }

        // 4. Deceleration — تقليل السرعة تدريجياً قبل الوصول
        float remaining = targetTicks - enc.avgTicks;
        float speed_factor = 1.0f;
        if (remaining < DECEL_ZONE)
        {
            speed_factor = max(MIN_SPEED_FACTOR, remaining / DECEL_ZONE);
        }
        int currentSpeed = (int)(baseSpeed * speed_factor);
        if (currentSpeed < 50)
        {
            currentSpeed = 50; // ضمان حد أدنى لسرعة المحركات لمنع التعليق
        }

        // 5. Determine steering error
        float steer_error = 0;
        bool hasLeft = (currentTof.leftBack < WALL_VALID_DIST);
        bool hasRight = (currentTof.rightBack < WALL_VALID_DIST);

        if (hasLeft && hasRight)
        {
            // Center between both walls
            steer_error = currentTof.leftBack - currentTof.rightBack;
        }
        else if (hasLeft)
        {
            // Follow left wall
            steer_error = (currentTof.leftBack - IDEAL_SIDE_DIST) * 1.5f;
        }
        else if (hasRight)
        {
            // Follow right wall
            steer_error = (IDEAL_SIDE_DIST - currentTof.rightBack) * 1.5f;
        }
        else
        {
            // Open corridor: Maintain straight heading using IMU Yaw
            float yaw_error = straightYaw - getYaw();
            steer_error = yaw_error * 3.5f;
        }

        // PID on steer_error (using local variables)
        local_steer_integral += steer_error;
        local_steer_integral = constrain(local_steer_integral, -50.0f, 50.0f);
        float steer_derivative = steer_error - local_steer_prev_error;
        local_steer_prev_error = steer_error;

        float steer_power = (steer_kp * steer_error) + (steer_ki * local_steer_integral) + (steer_kd * steer_derivative);
        steer_power = constrain(steer_power, -70.0f, 70.0f);

        int leftPwm = currentSpeed - (int)steer_power;
        int rightPwm = currentSpeed + (int)steer_power;

        setMotorSpeeds(leftPwm, rightPwm);
    }

    stopMotors();
    delay(40);
}