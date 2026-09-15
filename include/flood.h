#pragma once

int get_robot_x();
int get_robot_y();
void set_robot_x(int x);
void set_robot_y(int y);
bool isValid(int x, int y);
void set_robot_dir(int dir);
int get_robot_dir();
void setWall(int x, int y, int d);
bool hasWall(int x, int y, int d);
bool updateWalls(bool front, bool right, bool left);
void advanceRobotCoordinates();
void initMaze();
void flood(bool to_center = true);
int getNextMove();
bool isCenter(int x, int y);
void printMaze(int robotX, int robotY, int robotDir);
void exploreToCenter();
void exploreToStart();
