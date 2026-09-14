#include <Wire.h>
#include "Adafruit_VL53L0X.h"

#define SENSORS_COUNT 5

// دبابيس XSHUT للحساسات الـ 5
const int xshutPins[SENSORS_COUNT] = {13, 14, 26, 33, 16};

// العناوين الجديدة المميزة للحساسات
const int sensorAddresses[SENSORS_COUNT] = {0x30, 0x31, 0x32, 0x33, 0x34};

// إنشاء مصفوفة الحساسات
Adafruit_VL53L0X lox[SENSORS_COUNT];

void setupTof()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(1);
    }

    Serial.println(F("Initializing 5x VL53L0X sensors..."));

    // 1. إعداد جميع الدبابيس كمخرجات وإغلاق جميع الحساسات
    for (int i = 0; i < SENSORS_COUNT; i++)
    {
        pinMode(xshutPins[i], OUTPUT);
        digitalWrite(xshutPins[i], LOW);
    }
    delay(10); // وقت كافٍ لتأكيد إيقاف الحساسات تماماً

    // 2. تفعيل كل حساس بالترتيب وتغيير عنوانه (مثل الكود الشغال)
    for (int i = 0; i < SENSORS_COUNT; i++)
    {
        // تشغيل الحساس الحالي فقط
        digitalWrite(xshutPins[i], HIGH);
        delay(10);

        // تهيئة الحساس وتخصيص العنوان الجديد له
        if (!lox[i].begin(sensorAddresses[i]))
        {
            Serial.print(F("Failed to boot VL53L0X sensor #"));
            Serial.println(i + 1);
            while (1)
                ;
        }

        Serial.print(F("Sensor "));
        Serial.print(i + 1);
        Serial.println(F(" initialized successfully."));
    }

    Serial.println(F("All 5 sensors ready!\n"));
}

void readTof()
{
    VL53L0X_RangingMeasurementData_t measure;

    for (int i = 0; i < SENSORS_COUNT; i++)
    {
        // أخذ القراءة من الحساس الحالي
        lox[i].rangingTest(&measure, false);

        Serial.print(F("S"));
        Serial.print(i + 1);
        Serial.print(F(": "));

        if (measure.RangeStatus != 4)
        { // Status 4 تعني خارج المدى
            Serial.print(measure.RangeMilliMeter);
            Serial.print(F("mm"));
        }
        else
        {
            Serial.print(F("Out of range"));
        }

        if (i < SENSORS_COUNT - 1)
        {
            Serial.print(F("  |  "));
        }
    }
    Serial.println();

    delay(100); // مهلة بين القراءات
}