#pragma once

bool isValid(int x, int y);
void setWall(int x, int y, int d);
void initMaze();
void flood(bool to_center = true);
int getNextMove();
bool isCenter(int x, int y);

void exploreToCenter();
void exploreToStart();
