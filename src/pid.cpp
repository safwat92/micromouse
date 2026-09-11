float kp = 0.1, ki = .01, kd = 0.05;
float integral = 0;
float prev_error = 0;

float pid(float curr, float target) {
    float error = target - curr;

    integral += error;

    float derivative = (error - prev_error);

    prev_error = error;

    return (kp * error) + (ki * integral) + (kd * derivative);
}

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