#include <Wire.h>
#include <Arduino.h>
#include "mpu.h"
#include "motors.h"
#include "flood.h"

#define SDA_PIN 21
#define SCL_PIN 22
#define MPU_ADDR 0x68

// معامل التحويل القياسي لـ GyroZ عند مدى +/- 250 dps (نقسم القيمة الخام على 131.0 للحصول على deg/sec)
#define GYRO_SCALE 131.0

int16_t GyroZ_Raw = 0;
float gyroZ_offset = 0;
float currentYaw = 0;
unsigned long lastYawTime = 0;
unsigned long lastPrint = 0;

uint8_t actual_mpu_addr = 0x68;
bool mpu_detected = false;

void setupMPUDirect()
{
    Serial.println(F("\n--- [ Initializing MPU6050 IMU ] ---"));

    // 1. محاولة الاتصال بالعنوان 0x68 أولاً
    actual_mpu_addr = 0x68;

    // 2. إيقاظ الـ MPU6050 فوراً (إلغاء وضع النوم Sleep Mode)
    Wire.beginTransmission(actual_mpu_addr);
    Wire.write(0x6B); // Power Management Register 1
    Wire.write(0x01); // Wake up & set clock source to Gyro X (أكثر استقراراً)
    byte error = Wire.endTransmission(true);

    if (error != 0)
    {
        // محاولة التجربة على العنوان 0x69 (في حال كان AD0 واصل على HIGH)
        actual_mpu_addr = 0x69;
        Wire.beginTransmission(actual_mpu_addr);
        Wire.write(0x6B);
        Wire.write(0x01);
        error = Wire.endTransmission(true);
    }

    if (error == 0)
    {
        mpu_detected = true;
        Serial.print(F("[MPU] Successfully Woken Up & Detected at Address: 0x"));
        Serial.println(actual_mpu_addr, HEX);
    }
    else
    {
        mpu_detected = false;
        Serial.println(F("[!] ERROR: MPU6050 NOT responding on I2C bus! Check VCC/GND/SDA/SCL."));
        return;
    }

    // 3. ضبط مدى الجايروسكوب على +/- 250 dps
    Wire.beginTransmission(actual_mpu_addr);
    Wire.write(0x1B); // GYRO_CONFIG Register
    Wire.write(0x00); // FS_SEL = 0 (+/- 250 deg/s)
    Wire.endTransmission(true);

    delay(50);

    // 4. معايرة الـ Gyro Offset (تأكدي من ثبات الروبوت!)
    Serial.println(F("Calibrating MPU6050 GyroZ... Keep robot completely still!"));
    long sum = 0;
    int samples = 500;

    for (int i = 0; i < samples; i++)
    {
        Wire.beginTransmission(actual_mpu_addr);
        Wire.write(0x47);
        Wire.endTransmission(true);
        if (Wire.requestFrom((uint8_t)actual_mpu_addr, (uint8_t)2, (uint8_t)1) == 2)
        {
            int16_t gz = (Wire.read() << 8) | Wire.read();
            sum += gz;
        }
        delay(2);
    }

    gyroZ_offset = (float)sum / samples;
    Serial.print(F("GyroZ Offset Calibrated: "));
    Serial.println(gyroZ_offset);

    lastYawTime = micros();
}

void updateYaw()
{
    if (!mpu_detected)
        return;

    unsigned long now = micros();
    float dt = (now - lastYawTime) / 1000000.0;
    lastYawTime = now;

    if (dt <= 0 || dt > 0.5)
        dt = 0.001;

    // Try up to 2 attempts to read GyroZ
    for (int attempt = 0; attempt < 2; attempt++)
    {
        Wire.beginTransmission(actual_mpu_addr);
        Wire.write(0x47);
        // Use true (stop bit) — releases bus between write and read.
        // Repeated start (false) causes contention on shared bus with ToF sensors.
        if (Wire.endTransmission(true) != 0)
        byte writeErr = Wire.endTransmission(true);
        if (writeErr != 0)
        {
            static unsigned long lastMpuWarn = 0;
            if (millis() - lastMpuWarn > 2000)
            {
                lastMpuWarn = millis();
                Serial.print(F("[I2C FAIL -> MPU6050 (0x"));
                Serial.print(actual_mpu_addr, HEX);
                Serial.print(F(")] Register write failed (Error "));
                Serial.print(writeErr);
                Serial.println(F(")!"));
            }
            continue;
        }

        uint8_t n = Wire.requestFrom((uint8_t)actual_mpu_addr, (uint8_t)2, (uint8_t)1);
        if (n == 2)
        {
            GyroZ_Raw = (Wire.read() << 8) | Wire.read();

            float gyroZ_corrected = GyroZ_Raw - gyroZ_offset;
            float gyroZ_dps = gyroZ_corrected / GYRO_SCALE;

            if (abs(gyroZ_dps) > 0.5)
            {
                currentYaw += gyroZ_dps * dt;
            }
            return; // success — exit
        }
        else
        {
            static unsigned long lastReqWarn = 0;
            if (millis() - lastReqWarn > 2000)
            {
                lastReqWarn = millis();
                Serial.print(F("[I2C FAIL -> MPU6050 (0x"));
                Serial.print(actual_mpu_addr, HEX);
                Serial.println(F(")] requestFrom TIMEOUT (Error 263)!"));
            }
        }
        // Flush any partial data
        while (Wire.available())
            Wire.read();
    }
}

float getYaw()
{
    return currentYaw;
}

void resetYaw()
{
    currentYaw = 0;
    lastYawTime = micros();
}

void printMPU()
{
    if (millis() - lastPrint >= 150)
    {
        lastPrint = millis();
        Serial.print("GyroZ Raw: ");
        Serial.print(GyroZ_Raw);
        Serial.print(" | Current Yaw Angle: ");
        Serial.print(currentYaw);
        Serial.println(" deg");
    }
}

void turnToRelativeAngle(float deltaAngle)
{
    if (!mpu_detected)
    {
        Serial.println(F("[WARN] MPU not detected! Cannot turn with gyro."));
        return;
    }

    float startYaw = getYaw();
    float targetYaw = startYaw + deltaAngle;

    const float Kp = 2.4;
    const float Kd = 0.12;
    const float tolerance = 1.5;
    const int minSpeed = 80;
    const int maxSpeed = 180;

    float prevError = deltaAngle;
    unsigned long startTime = millis();
    unsigned long timeout = 500 + abs((int)deltaAngle) * 15;
    int stableCount = 0;

    while (millis() - startTime < timeout)
    {
        updateYaw();
        float currentAngle = getYaw();
        float error = targetYaw - currentAngle;

        if (abs(error) <= tolerance)
        {
            stableCount++;
            if (stableCount >= 5)
                break;
        }
        else
        {
            stableCount = 0;
        }

        float derivative = error - prevError;
        prevError = error;

        float turnPower = (Kp * error) + (Kd * derivative);

        if (abs(turnPower) < minSpeed)
        {
            turnPower = (turnPower > 0) ? minSpeed : -minSpeed;
        }

        turnPower = constrain(turnPower, -maxSpeed, maxSpeed);

        setMotorSpeeds(-turnPower, turnPower);
        delay(10);
    }

    stopMotors();
    delay(40);
}

void turnToDirection(int target_dir)
{
    int curr = get_robot_dir();
    int diff = (target_dir - curr + 4) % 4;

    if (diff == 1)
    {
        // Turn Right 90 degrees
        turnToRelativeAngle(-90.0f);
    }
    else if (diff == 2)
    {
        // Turn 180 degrees
        turnToRelativeAngle(180.0f);
    }
    else if (diff == 3)
    {
        // Turn Left 90 degrees
        turnToRelativeAngle(90.0f);
    }

    set_robot_dir(target_dir);
    // Reset الـ yaw بعد اللفة عشان نبدأ من صفر — ده بيمنع تراكم الـ gyro drift
    resetYaw();
}