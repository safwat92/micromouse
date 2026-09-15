#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include "i2c_diag.h"
#include "tof.h"

#define SDA_PIN 21
#define SCL_PIN 22

struct SensorInfo
{
    uint8_t address;
    const char *name;
    int xshutPin;
};

const SensorInfo EXPECTED_SENSORS[] = {
    {0x68, "MPU6050 IMU", -1},
    {0x30, "ToF #1 (Front)", 4},
    {0x31, "ToF #2 (RightFront)", 13},
    {0x32, "ToF #3 (LeftFront)", 17},
    {0x33, "ToF #4 (RightBack)", 16},
    {0x34, "ToF #5 (LeftBack)", 23}
};

const int TOTAL_EXPECTED = sizeof(EXPECTED_SENSORS) / sizeof(EXPECTED_SENSORS[0]);

extern Adafruit_VL53L0X lox[5];
extern bool tofInitialized[5];

void printI2CBusLinesState()
{
    int sda = digitalRead(SDA_PIN);
    int scl = digitalRead(SCL_PIN);

    Serial.println(F("\n--- [ I2C Physical Bus Lines Status ] ---"));
    Serial.print(F("SDA (GPIO 21): "));
    Serial.print(sda == HIGH ? F("HIGH (Idle/Normal)") : F("LOW (BUS HUNG / SHORTED TO GND!)"));
    Serial.print(F(" | SCL (GPIO 22): "));
    Serial.println(scl == HIGH ? F("HIGH (Idle/Normal)") : F("LOW (CLOCK HUNG / CLOCK STRETCHING!)"));

    if (sda == LOW || scl == LOW)
    {
        Serial.println(F("[!] WARNING: One of the I2C lines is stuck LOW! This causes Error 263 (ESP_ERR_TIMEOUT)."));
    }
}

void scanAndDiagnoseI2C()
{
    Serial.println(F("\n================ [ I2C BUS SCAN & DIAGNOSTIC ] ================"));
    printI2CBusLinesState();

    byte error, address;
    int nDevices = 0;
    bool foundDefault29 = false;

    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.print(F(" -> Device detected at address 0x"));
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);

            if (address == 0x29)
            {
                foundDefault29 = true;
                Serial.print(F(" [!] CRITICAL: VL53L0X Default Address! (A sensor lost power & reset back to 0x29!)"));
            }
            else if (address == 0x68)
            {
                Serial.print(F(" [MPU6050 IMU (Primary)]"));
            }
            else if (address == 0x69)
            {
                Serial.print(F(" [MPU6050 IMU (Secondary AD0=HIGH)]"));
            }
            else if (address == 0x30)
            {
                Serial.print(F(" [ToF #1: Front (Pin 4)]"));
            }
            else if (address == 0x31)
            {
                Serial.print(F(" [ToF #2: RightFront (Pin 13)]"));
            }
            else if (address == 0x32)
            {
                Serial.print(F(" [ToF #3: LeftFront (Pin 17)]"));
            }
            else if (address == 0x33)
            {
                Serial.print(F(" [ToF #4: RightBack (Pin 16)]"));
            }
            else if (address == 0x34)
            {
                Serial.print(F(" [ToF #5: LeftBack (Pin 23)]"));
            }
            Serial.println();
            nDevices++;
        }
        else if (error == 4 || error == 5)
        {
            Serial.print(F(" -> [TIMEOUT/ERROR "));
            Serial.print(error);
            Serial.print(F("] at address 0x"));
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }

    Serial.println(F("---------------------------------------------------------------"));
    Serial.print(F("Total active devices found: "));
    Serial.println(nDevices);

    // التحقق من كل حساس متوقع
    Serial.println(F("\n--- [ Expected Sensors Checklist ] ---"));
    for (int i = 0; i < TOTAL_EXPECTED; i++)
    {
        Wire.beginTransmission(EXPECTED_SENSORS[i].address);
        byte err = Wire.endTransmission();

        Serial.print(F(" - "));
        Serial.print(EXPECTED_SENSORS[i].name);
        Serial.print(F(" (0x"));
        Serial.print(EXPECTED_SENSORS[i].address, HEX);
        if (EXPECTED_SENSORS[i].xshutPin != -1)
        {
            Serial.print(F(", Pin "));
            Serial.print(EXPECTED_SENSORS[i].xshutPin);
        }
        Serial.print(F("): "));

        if (err == 0)
        {
            Serial.println(F("[CONNECTED & RESPONDING - OK]"));
        }
        else if (err == 2)
        {
            Serial.println(F("[FAILED: NACK - No device found at this address!]"));
        }
        else if (err == 5 || err == 4)
        {
            Serial.println(F("[FAILED: TIMEOUT (Error 263) - Sensor hung the bus!]"));
        }
        else
        {
            Serial.print(F("[FAILED: I2C Error Code "));
            Serial.print(err);
            Serial.println(F("]"));
        }
    }

    if (foundDefault29)
    {
        Serial.println(F("\n[!] WARNING: Address 0x29 is ACTIVE!"));
        Serial.println(F("    This means one of your VL53L0X sensors experienced a voltage dip (Brownout)"));
        Serial.println(F("    and rebooted back to factory address 0x29, causing Error 263 when read!"));
    }
    Serial.println(F("===============================================================\n"));
}

void runFullI2CHardwareTest()
{
    Serial.println(F("\n==============================================================="));
    Serial.println(F(">>> STARTING DEEP I2C SENSOR-BY-SENSOR DIAGNOSTIC TEST <<<"));
    Serial.println(F("==============================================================="));

    // 1. اختبار الـ MPU6050
    Serial.println(F("\n[TEST 1/2] Testing MPU6050 IMU (0x68)..."));
    Wire.beginTransmission(0x68);
    byte mpuPing = Wire.endTransmission();

    if (mpuPing != 0)
    {
        Serial.print(F(" -> [CRITICAL FAIL] MPU6050 did NOT ACK at 0x68! Error: "));
        Serial.println(mpuPing);
        Serial.println(F("    >>> THIS SENSOR IS CAUSING I2C FAILURES! Check its VCC/GND/SDA/SCL."));
    }
    else
    {
        Serial.print(F(" -> MPU6050 ACK OK! Attempting to read GyroZ register 0x47... "));
        Wire.beginTransmission(0x68);
        Wire.write(0x47);
        byte writeErr = Wire.endTransmission(true);

        if (writeErr != 0)
        {
            Serial.print(F("[FAIL] Register write failed with error: "));
            Serial.println(writeErr);
        }
        else
        {
            unsigned long t0 = millis();
            uint8_t n = Wire.requestFrom((uint8_t)0x68, (uint8_t)2, (uint8_t)1);
            unsigned long elapsed = millis() - t0;

            if (n == 2)
            {
                int16_t gz = (Wire.read() << 8) | Wire.read();
                Serial.print(F("[SUCCESS] Read in "));
                Serial.print(elapsed);
                Serial.print(F("ms | Raw GyroZ: "));
                Serial.println(gz);
            }
            else
            {
                Serial.print(F("[FAIL - ERROR 263 TIMEOUT] requestFrom timed out after "));
                Serial.print(elapsed);
                Serial.println(F("ms! MPU6050 is not returning data!"));
            }
        }
    }

    // 2. اختبار حساسات الـ ToF الخمسة بالتفصيل
    Serial.println(F("\n[TEST 2/2] Testing 5x VL53L0X ToF Sensors individually..."));
    const char *tofNames[5] = {
        "ToF #1 (Front, Pin 4, 0x30)",
        "ToF #2 (RightFront, Pin 13, 0x31)",
        "ToF #3 (LeftFront, Pin 17, 0x32)",
        "ToF #4 (RightBack, Pin 16, 0x33)",
        "ToF #5 (LeftBack, Pin 23, 0x34)"
    };
    const uint8_t tofAddrs[5] = {0x30, 0x31, 0x32, 0x33, 0x34};

    for (int i = 0; i < 5; i++)
    {
        Serial.print(F(" -> Testing "));
        Serial.print(tofNames[i]);
        Serial.print(F("... "));

        // أ) اختبار الـ Ping
        Wire.beginTransmission(tofAddrs[i]);
        byte pingErr = Wire.endTransmission();

        if (pingErr != 0)
        {
            Serial.print(F("[PING FAILED] Error: "));
            Serial.print(pingErr);
            if (pingErr == 2)
                Serial.println(F(" (NACK: Sensor not present at this address!)"));
            else if (pingErr == 5 || pingErr == 4)
                Serial.println(F(" (TIMEOUT / Error 263: Sensor hung the bus!)"));
            else
                Serial.println();
            continue;
        }

        // ب) اختبار قراءة المسافة الفعلية
        if (!tofInitialized[i])
        {
            Serial.println(F("[WARNING: Sensor was marked NOT INITIALIZED during setup!]"));
            continue;
        }

        unsigned long t0 = millis();
        VL53L0X_RangingMeasurementData_t measure;
        lox[i].rangingTest(&measure, false);
        unsigned long elapsed = millis() - t0;

        if (measure.RangeStatus != 4)
        {
            Serial.print(F("[OK] Distance: "));
            Serial.print(measure.RangeMilliMeter);
            Serial.print(F(" mm (took "));
            Serial.print(elapsed);
            Serial.println(F(" ms)"));
        }
        else
        {
            Serial.print(F("[OUT OF RANGE / Status 4] (took "));
            Serial.print(elapsed);
            Serial.println(F(" ms)"));
        }
    }

    // 3. فحص هل هناك حساس أعاد ضبط نفسه إلى 0x29
    Wire.beginTransmission(0x29);
    byte err29 = Wire.endTransmission();
    if (err29 == 0)
    {
        Serial.println(F("\n[!] CRITICAL WARNING: A sensor is answering at 0x29!"));
        Serial.println(F("    This PROVES that one of the ToF sensors lost power and reset!"));
        Serial.println(F("    When motors draw current, the 3.3V power drops, resetting the VL53L0X."));
    }

    Serial.println(F("\n================ [ DIAGNOSTIC TEST COMPLETE ] ================\n"));
}
