#include <Wire.h>
#include <Arduino.h>

#define SDA_PIN 21
#define SCL_PIN 22
#define MPU_ADDR 0x68

int16_t AccX, AccY, AccZ;
int16_t GyroX, GyroY, GyroZ;

unsigned long lastPrint = 0;

void setupMPUDirect()
{
    Wire.begin(SDA_PIN, SCL_PIN);

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0);
    Wire.endTransmission(true);
}

void readRawMPU()
{
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);            // البدء من سجل ACCEL_XOUT_H
    Wire.endTransmission(false); // إرسال Repeated Start لتعديل العنوان دون تحرير الناقل

    // طلب 14 بايت مع تحديد الأنواع بشكل صريح لتفادي التحذيرات
    uint8_t bytesReceived = Wire.requestFrom((uint8_t)MPU_ADDR, (size_t)14, true);

    if (bytesReceived == 14)
    {
        // قراءة البايت العالي والمنخفض بالترتيب الصحيح والضم بـ Bit Shift
        uint8_t rawAccX_H = Wire.read();
        uint8_t rawAccX_L = Wire.read();
        AccX = (int16_t)((rawAccX_H << 8) | rawAccX_L);

        uint8_t rawAccY_H = Wire.read();
        uint8_t rawAccY_L = Wire.read();
        AccY = (int16_t)((rawAccY_H << 8) | rawAccY_L);

        uint8_t rawAccZ_H = Wire.read();
        uint8_t rawAccZ_L = Wire.read();
        AccZ = (int16_t)((rawAccZ_H << 8) | rawAccZ_L);

        uint8_t rawTemp_H = Wire.read();
        uint8_t rawTemp_L = Wire.read();
        int16_t Temp = (int16_t)((rawTemp_H << 8) | rawTemp_L); // درجة الحرارة (غير مستخدمة)

        uint8_t rawGyroX_H = Wire.read();
        uint8_t rawGyroX_L = Wire.read();
        GyroX = (int16_t)((rawGyroX_H << 8) | rawGyroX_L);

        uint8_t rawGyroY_H = Wire.read();
        uint8_t rawGyroY_L = Wire.read();
        GyroY = (int16_t)((rawGyroY_H << 8) | rawGyroY_L);

        uint8_t rawGyroZ_H = Wire.read();
        uint8_t rawGyroZ_L = Wire.read();
        GyroZ = (int16_t)((rawGyroZ_H << 8) | rawGyroZ_L);
    }
}

void printMPU()
{
    if (millis() - lastPrint >= 150)
    {
        lastPrint = millis();
        Serial.print(" | AccelX: ");
        Serial.print(AccX);
        Serial.print(" AccelY: ");
        Serial.print(AccY);
        Serial.print(" AccelZ: ");
        Serial.print(AccZ);
        Serial.print(" | GyroZ: ");
        Serial.println(GyroZ);
    }
}