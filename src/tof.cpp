#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include <tof.h>

#define SENSORS_COUNT 5
#define WALL_THRESHOLD 80
#define MAX_VALID_RANGE 800 // 80 cm

const int xshutPins[SENSORS_COUNT] = {4, 13, 17, 16, 23};
const int sensorAddresses[SENSORS_COUNT] = {0x30, 0x31, 0x32, 0x33, 0x34};

Adafruit_VL53L0X lox[SENSORS_COUNT];
TofData currentTof;
bool tofInitialized[SENSORS_COUNT] = {false};

void setupTof()
{
    Serial.println(F("\n--- [ Initializing VL53L0X Sensors ] ---"));

    // 1. إيقاف كل الحساسات مؤقتاً
    for (int i = 0; i < SENSORS_COUNT; i++)
    {
        pinMode(xshutPins[i], OUTPUT);
        digitalWrite(xshutPins[i], LOW);
    }
    delay(50);

    // 2. تشغيل الحساسات حساس تلو الآخر وتغيير العنوان
    for (int i = 0; i < SENSORS_COUNT; i++)
    {
        Serial.print(F("Enabling ToF #"));
        Serial.print(i + 1);
        Serial.print(F(" on Pin "));
        Serial.print(xshutPins[i]);
        Serial.print(F("... "));

        // تشغيل الحساس الحالي فقط
        digitalWrite(xshutPins[i], HIGH);
        delay(30);

        // التحقق من استجابة الحساس على العنوان الافتراضي 0x29 قبل محاولة التهيئة
        Wire.beginTransmission(0x29);
        byte ping29 = Wire.endTransmission();
        if (ping29 != 0)
        {
            tofInitialized[i] = false;
            Serial.print(F("FAILED (No response at 0x29, I2C Error "));
            Serial.print(ping29);
            Serial.println(F(" - Check VCC/GND/XSHUT wiring!)"));
            continue;
        }

        // تهيئة الحساس وعزله على عنوانه الجديد
        if (lox[i].begin(sensorAddresses[i], false, &Wire))
        {
            tofInitialized[i] = true;
            Serial.print(F("SUCCESS (Address: 0x"));
            Serial.print(sensorAddresses[i], HEX);
            Serial.println(F(")"));
        }
        else
        {
            tofInitialized[i] = false;
            Serial.println(F("FAILED during begin()!"));
        }
        delay(10);
    }
    Serial.println(F("----------------------------------------\n"));
}

void updateTofData()
{
    VL53L0X_RangingMeasurementData_t measure;
    float tempDist[SENSORS_COUNT];

    for (int i = 0; i < SENSORS_COUNT; i++)
    {
        if (tofInitialized[i])
        {
            // فحص وجود الحساس على عنوانه قبل محاولة القراءة لمنع تعليق الناقل
            Wire.beginTransmission(sensorAddresses[i]);
            byte pingErr = Wire.endTransmission();
            if (pingErr != 0)
            {
                static unsigned long lastTofWarn[5] = {0};
                if (millis() - lastTofWarn[i] > 2000)
                {
                    lastTofWarn[i] = millis();
                    Serial.print(F("[I2C FAIL -> ToF #"));
                    Serial.print(i + 1);
                    Serial.print(F(" (Pin "));
                    Serial.print(xshutPins[i]);
                    Serial.print(F(", 0x"));
                    Serial.print(sensorAddresses[i], HEX);
                    Serial.print(F(")] Not responding (Error "));
                    Serial.print(pingErr);
                    Serial.println(F(")!"));
                }
                tempDist[i] = MAX_VALID_RANGE;
                continue;
            }

            lox[i].rangingTest(&measure, false);

            if (measure.RangeStatus != 4 && measure.RangeMilliMeter < MAX_VALID_RANGE)
            {
                tempDist[i] = measure.RangeMilliMeter;
            }
            else
            {
                tempDist[i] = MAX_VALID_RANGE;
            }
        }
        else
        {
            tempDist[i] = MAX_VALID_RANGE;
        }
    }

    currentTof.leftBack = tempDist[4];
    currentTof.leftFront = tempDist[2];
    currentTof.front = tempDist[0];
    currentTof.rightFront = tempDist[1];
    currentTof.rightBack = tempDist[3];

    currentTof.hasLeftWall = (currentTof.leftFront < WALL_THRESHOLD || currentTof.leftBack < WALL_THRESHOLD);
    currentTof.hasFrontWall = (currentTof.front < WALL_THRESHOLD);
    currentTof.hasRightWall = (currentTof.rightFront < WALL_THRESHOLD || currentTof.rightBack < WALL_THRESHOLD);
}

// دالة سريعة تقرأ 3 حساسات بس (الأمامي + الجانبيين الخلفيين)
// بتستخدم جوه driveOneCell عشان تسرّع الـ PID loop
// الحساسات: 0=front, 3=rightBack, 4=leftBack
void updateTofSteering()
{
    VL53L0X_RangingMeasurementData_t measure;
    const int steeringIdx[] = {0, 3, 4}; // front, rightBack, leftBack

    for (int j = 0; j < 3; j++)
    {
        int i = steeringIdx[j];
        float dist = MAX_VALID_RANGE;

        if (tofInitialized[i])
        {
            lox[i].rangingTest(&measure, false);

            if (measure.RangeStatus != 4 && measure.RangeMilliMeter < MAX_VALID_RANGE)
            {
                dist = measure.RangeMilliMeter;
            }
        }

        if (i == 0)
            currentTof.front = dist;
        else if (i == 3)
            currentTof.rightBack = dist;
        else if (i == 4)
            currentTof.leftBack = dist;
    }

    currentTof.hasFrontWall = (currentTof.front < WALL_THRESHOLD);
    // تحديث wall flags بناءً على الحساسات المتاحة
    currentTof.hasLeftWall = (currentTof.leftBack < WALL_THRESHOLD);
    currentTof.hasRightWall = (currentTof.rightBack < WALL_THRESHOLD);
}