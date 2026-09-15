#pragma once
#include <Arduino.h>

// دالة فحص وتشخيص شاملة لجميع الحساسات على ناقل الـ I2C
// تكشف بالضبط أي حساس لا يستجيب أو يسبب Timeout (Error 263)
void scanAndDiagnoseI2C();

// فحص حالة خطوط الـ SDA والـ SCL (هل يوجد خط معلق على الـ LOW)
void printI2CBusLinesState();

// اختبار شامل وتشخيص تفاعلي لكل حساس على حدة (MPU6050 + 5x ToF)
void runFullI2CHardwareTest();
