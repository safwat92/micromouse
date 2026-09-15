#include <queue>
#include <Arduino.h>

#define MAZE_WIDTH 8
#define MAZE_HEIGHT 8
#define MAZE_SIZE 8

const int NORTH = 0;
const int EAST = 1;
const int SOUTH = 2;
const int WEST = 3;

const int dir_x[4] = {0, 1, 0, -1};
const int dir_y[4] = {1, 0, -1, 0};
const char dir_chars[4] = {'n', 'e', 's', 'w'};

int robot_x = 7;
int robot_y = 0;
int curr_dir = NORTH;

int maze[MAZE_WIDTH][MAZE_HEIGHT];
bool walls[MAZE_WIDTH][MAZE_HEIGHT][4] = {false};

int get_robot_x()
{
    return robot_x;
}
int get_robot_y()
{
    return robot_y;
}
void set_robot_x(int x)
{
    robot_x = x;
}
void set_robot_y(int y)
{
    robot_y = y;
}
int get_robot_dir()
{
    return curr_dir;
}
void set_robot_dir(int dir)
{
    curr_dir = dir;
}

bool isValid(int x, int y)
{
    return (x >= 0 && x < MAZE_WIDTH && y >= 0 && y < MAZE_HEIGHT);
}

void setWall(int x, int y, int d)
{
    walls[x][y][d] = true;

    // set the wall in the opposite direction of the cell
    int next_x = x + dir_x[d];
    int next_y = y + dir_y[d];
    if (isValid(next_x, next_y))
    {
        int opp_d = (d + 2) % 4;
        walls[next_x][next_y][opp_d] = true;
    }
}

void initMaze()
{
    // set the outer maze walls
    for (int x = 0; x < MAZE_WIDTH; x++)
    {
        walls[x][0][SOUTH] = true;
        walls[x][MAZE_HEIGHT - 1][NORTH] = true;
    }
    for (int y = 0; y < MAZE_HEIGHT; y++)
    {
        walls[0][y][WEST] = true;
        walls[MAZE_WIDTH - 1][y][EAST] = true;
    }
}

bool hasWall(int x, int y, int d)
{
    if (isValid(x, y) && d >= 0 && d < 4)
    {
        return walls[x][y][d];
    }
    return true;
}

bool updateWalls(bool front, bool right, bool left)
{
    bool changed = false;
    int front_dir = curr_dir;
    int right_dir = (curr_dir + 1) % 4;
    int left_dir = (curr_dir + 3) % 4;

    if (front && !walls[robot_x][robot_y][front_dir])
    {
        setWall(robot_x, robot_y, front_dir);
        changed = true;
    }
    if (right && !walls[robot_x][robot_y][right_dir])
    {
        setWall(robot_x, robot_y, right_dir);
        changed = true;
    }
    if (left && !walls[robot_x][robot_y][left_dir])
    {
        setWall(robot_x, robot_y, left_dir);
        changed = true;
    }
    return changed;
}

void advanceRobotCoordinates()
{
    robot_x += dir_x[curr_dir];
    robot_y += dir_y[curr_dir];
}

void printMaze(int robotX, int robotY, int robotDir)
{
    Serial.println(F("\n================= CURRENT MAZE STATE ================="));

    char dirChar = '^';
    if (robotDir == NORTH)
        dirChar = '^';
    else if (robotDir == EAST)
        dirChar = '>';
    else if (robotDir == SOUTH)
        dirChar = 'v';
    else if (robotDir == WEST)
        dirChar = '<';

    for (int y = MAZE_HEIGHT - 1; y >= 0; y--)
    {
        // 1. طباعة الجدران الشمالية
        for (int x = 0; x < MAZE_WIDTH; x++)
        {
            Serial.print(F("+"));
            if (hasWall(x, y, NORTH))
            {
                Serial.print(F("---"));
            }
            else
            {
                Serial.print(F("   "));
            }
        }
        Serial.println(F("+"));

        // 2. طباعة الجدران الغربية والشرقية مع (قيم الخلايا أو موقع الروبوت)
        for (int x = 0; x < MAZE_WIDTH; x++)
        {
            if (hasWall(x, y, WEST))
            {
                Serial.print(F("|"));
            }
            else
            {
                Serial.print(F(" "));
            }

            // إذا كانت الخلية هي موقع الروبوت الحالي
            if (x == robotX && y == robotY)
            {
                Serial.print(F(" "));
                Serial.print(dirChar);
                Serial.print(F(" "));
            }
            else
            {
                // طباعة قيمة الـ Floodfill المخزنة في maze[x][y]
                int val = maze[x][y];

                if (val == -1)
                {
                    Serial.print(F(" ? ")); // خلية لم يتم الوصول إليها بعد
                }
                else if (val < 10)
                {
                    Serial.print(F(" "));
                    Serial.print(val);
                    Serial.print(F(" ")); // رقم من خانة واحدة (مثال: " 3 ")
                }
                else if (val < 100)
                {
                    Serial.print(val);
                    Serial.print(F(" ")); // رقم من خانتين (مثال: "12 ")
                }
                else
                {
                    Serial.print(val); // رقم من 3 خانات
                }
            }
        }

        // الجدار الشرقي للخلية الأخيرة في الصف
        if (hasWall(MAZE_WIDTH - 1, y, EAST))
        {
            Serial.println(F("|"));
        }
        else
        {
            Serial.println(F(" "));
        }
    }

    // 3. طباعة الجدار السفلي الأخير للمتاهة (SOUTH)
    for (int x = 0; x < MAZE_WIDTH; x++)
    {
        Serial.print(F("+"));
        if (hasWall(x, 0, SOUTH))
        {
            Serial.print(F("---"));
        }
        else
        {
            Serial.print(F("   "));
        }
    }
    Serial.println(F("+"));
    Serial.println(F("======================================================\n"));
}

void flood(bool to_center = true)
{
    for (int x = 0; x < MAZE_WIDTH; x++)
    {
        for (int y = 0; y < MAZE_HEIGHT; y++)
        {
            maze[x][y] = -1;
        }
    }

    std::queue<std::pair<int, int>> q;

    if (to_center)
    {
        int mid_x1 = MAZE_WIDTH / 2 - 1;
        int mid_x2 = MAZE_WIDTH / 2;
        int mid_y1 = MAZE_HEIGHT / 2 - 1;
        int mid_y2 = MAZE_HEIGHT / 2;

        for (int x = mid_x1; x <= mid_x2; x++)
        {
            for (int y = mid_y1; y <= mid_y2; y++)
            {
                maze[x][y] = 0;
                q.push({x, y});
            }
        }
    }
    else
    {
        maze[0][0] = 0;
        q.push({0, 0});
    }

    while (!q.empty())
    {
        auto [cx, cy] = q.front();
        q.pop();

        for (int d = 0; d < 4; d++)
        {
            int next_x = cx + dir_x[d];
            int next_y = cy + dir_y[d];

            if (isValid(next_x, next_y) && !walls[cx][cy][d] && maze[next_x][next_y] == -1)
            {
                maze[next_x][next_y] = maze[cx][cy] + 1;
                q.push({next_x, next_y});
            }
        }
    }
}

int getNextMove()
{
    int min_maze = 1e9;
    int best_dir = -1;

    for (int d = 0; d < 4; d++)
    {
        if (!walls[robot_x][robot_y][d])
        {
            int next_x = robot_x + dir_x[d];
            int next_y = robot_y + dir_y[d];

            if (isValid(next_x, next_y) && maze[next_x][next_y] != -1)
            {
                if (maze[next_x][next_y] < min_maze)
                {
                    min_maze = maze[next_x][next_y];
                    best_dir = d;
                }
                else if (maze[next_x][next_y] == min_maze && d == curr_dir)
                {
                    best_dir = d;
                }
            }
        }
    }
    return best_dir;
}

// void moveTo(int target_dir)
// {
//     int diff = (target_dir - curr_dir + 4) % 4;

//     if (diff == 1)
//     {
//         API::turnRight();
//     }
//     else if (diff == 2)
//     {
//         API::turnRight();
//         API::turnRight();
//     }
//     else if (diff == 3)
//     {
//         API::turnLeft();
//     }

//     API::moveForward();
//     curr_dir = target_dir;
//     robot_x += dir_x[curr_dir];
//     robot_y += dir_y[curr_dir];
// }

// التحقق من الوصول للسنتر
bool isCenter(int x, int y)
{
    int mid_x1 = MAZE_WIDTH / 2 - 1;
    int mid_x2 = MAZE_WIDTH / 2;
    int mid_y1 = MAZE_HEIGHT / 2 - 1;
    int mid_y2 = MAZE_HEIGHT / 2;
    return (x >= mid_x1 && x <= mid_x2 && y >= mid_y1 && y <= mid_y2);
}

// دالة الاستكشاف إلى السنتر
void exploreToCenter()
{
    // API::setColor(robot_x, robot_y, 'G'); // تلوين نقطة البداية بالأخضر

    while (true)
    {
        // 1. قراءة الحساسات وتسجيل الجدران
        // updateWalls();

        // 2. إعادة حساب المسافات
        flood(true);

        // 3. التوقف عند الوصول إلى السنتر
        if (isCenter(robot_x, robot_y))
        {
            // API::setColor(robot_x, robot_y, 'G');
            break;
        }

        // 4. تحديد أفضل اتجاه للحركة
        int next_dir = getNextMove();
        if (next_dir == -1)
        {
            // cerr << "Error: No reachable path!" << endl;
            break;
        }

        // 5. التحرك
        // moveTo(next_dir);
    }
}

// دالة العودة إلى البداية (0, 0)
void exploreToStart()
{
    while (true)
    {
        // updateWalls();
        flood(false);

        if (robot_x == 0 && robot_y == 0)
        {
            // API::setColor(0, 0, 'G');
            break;
        }

        int next_dir = getNextMove();
        if (next_dir == -1)
        {
            // cerr << "Error: No reachable path to start!" << endl;
            break;
        }

        // moveTo(next_dir);
    }
}

// int main()
// {
//     // تهيئة المتاهة
//     initMaze();

//     // المرحلة الأولى: الاستكشاف حتى الوصول للمنتصف (السنتر)
//     exploreToCenter();

//     // (اختياري) يمكنك تفعيل السطور التالية للعودة للبداية ثم عمل دورة سريعة
//     /*
//     exploreToStart();
//     exploreToCenter();
//     */

//     return 0;
// }