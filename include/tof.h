#pragma once

struct TofData
{
    float leftBack;
    float leftFront;
    float front;
    float rightFront;
    float rightBack;

    bool hasLeftWall;
    bool hasFrontWall;
    bool hasRightWall;
};

extern TofData currentTof;

void setupTof();
void updateTofData();
void updateTofSteering();