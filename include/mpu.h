#pragma once

void setupMPUDirect();
void updateYaw();
float getYaw();
void resetYaw();
void printMPU();

void turnToRelativeAngle(float deltaAngle);
void turnToDirection(int target_dir);