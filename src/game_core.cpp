#include "game_core.h"
#include <iostream>
#include <windows.h>
#include <cmath>
#include <string>
#include <conio.h>

using namespace std;

int playerX = WIDTH / 2;
int playerY = HEIGHT - 2;
int score = 0;
int level = 1;
bool gameOver = false;

ObjectPool<Bullet> bulletPool;
ObjectPool<Enemy> enemyPool;
ObjectPool<Particle> particlePool;

list<Bullet*> activeBullets;
list<Enemy*> activeEnemies;
list<Particle*> activeParticles;
vector<Star> backgroundStars;

const string COLOR_RESET = "\033[0m";
const string COLOR_PLAYER = "\033[92m";
const string COLOR_ENEMY_NORM = "\033[91m";
const string COLOR_ENEMY_HARD = "\033[95m";
const string COLOR_BULLET = "\033[93m";
const string COLOR_PARTICLE = "\033[38;5;208m";
const string COLOR_STAR_FAST = "\033[37m";
const string COLOR_STAR_SLOW = "\033[90m";

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

void InitGame() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    CONSOLE_CURSOR_INFO cursor_info = { 1, 0 };
    SetConsoleCursorInfo(hOut, &cursor_info);

    for (int i = 0; i < 30; i++) {
        backgroundStars.push_back({rand() % WIDTH, rand() % HEIGHT, (rand() % 2) + 1});
    }
}

void CreateExplosion(int x, int y, int scale) {
    int particleCount = (8 + rand() % 6) * scale;
    for (int i = 0; i < particleCount; i++) {
        Particle* p = particlePool.allocate();
        p->x = (float)x;
        p->y = (float)y;
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float speed = ((rand() % 15) + 5) / 10.0f;
        p->vx = cos(angle) * speed * 2.0f;
        p->vy = sin(angle) * speed;
        p->life = 4 + rand() % 6;
        activeParticles.push_back(p);
    }
}

void Draw() {
    COORD pos = { 0, 0 };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);

    struct Cell { char c; string color; };
    vector<vector<Cell>> grid(HEIGHT, vector<Cell>(WIDTH, {' ', COLOR_RESET}));

    for (auto& star : backgroundStars) {
        if(star.x >= 0 && star.x < WIDTH && star.y >= 0 && star.y < HEIGHT) {
            grid[star.y][star.x] = {'.', star.speed == 2 ? COLOR_STAR_FAST : COLOR_STAR_SLOW};
        }
    }

    for (auto p : activeParticles) {
        int px = (int)p->x; int py = (int)p->y;
        if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) grid[py][px] = {'*', COLOR_PARTICLE};
    }

    for (auto b : activeBullets) {
        if (b->x >= 0 && b->x < WIDTH && b->y >= 0 && b->y < HEIGHT) grid[b->y][b->x] = {'^', COLOR_BULLET};
    }

    for (auto e : activeEnemies) {
        if (e->x >= 0 && e->x < WIDTH && e->y >= 0 && e->y < HEIGHT) {
            char icon = (e->type == 1) ? 'W' : 'V';
            string col = (e->type == 1) ? COLOR_ENEMY_HARD : COLOR_ENEMY_NORM;
            grid[e->y][e->x] = {icon, col};
        }
    }

    grid[playerY][playerX] = {'A', COLOR_PLAYER};

    string buffer = "";
    for (int y = 0; y < HEIGHT; y++) {
        string lastColor = "";
        for (int x = 0; x < WIDTH; x++) {
            if (grid[y][x].color != lastColor) { buffer += grid[y][x].color; lastColor = grid[y][x].color; }
            buffer += grid[y][x].c;
        }
        buffer += COLOR_RESET + "\n";
    }

    cout << buffer;
    cout << COLOR_PLAYER << ">> 战绩: " << score << " | 难度系数: " << level << COLOR_RESET << endl;
    cout << "----------------------------------------\n";
    cout << COLOR_BULLET << "[内存池引擎实时状态]" << COLOR_RESET << "\n";
    cout << "子弹 | 活跃:" << activeBullets.size() << " " << "池存:" << bulletPool.free_count() << " " << "总New:" << bulletPool.total_count() << "\n";
    cout << COLOR_ENEMY_NORM << "敌机 | 活跃:" << activeEnemies.size() << " " << "池存:" << enemyPool.free_count() << " " << "总New:" << enemyPool.total_count() << "\n";
    cout << COLOR_PARTICLE << "粒子 | 活跃:" << activeParticles.size() << " " << "池存:" << particlePool.free_count() << " " << "总New:" << particlePool.total_count() << "\n" << COLOR_RESET;
    cout << "----------------------------------------\n";
}

void Input() {
    if (_kbhit()) {
        char c = _getch();
        if ((c == 'a' || c == 'A') && playerX > 0) playerX--;
        if ((c == 'd' || c == 'D') && playerX < WIDTH - 1) playerX++;
        if (c == ' ') {
            for(int i = -1; i <= 1; i++) {
                if (playerX + i >= 0 && playerX + i < WIDTH) {
                    Bullet* b = bulletPool.allocate();
                    b->x = playerX + i; b->y = playerY - 1;
                    activeBullets.push_back(b);
                }
            }
        }
    }
}

void Logic() {
    level = (score / 2000) + 1;

    for (auto& star : backgroundStars) {
        star.y += star.speed;
        if (star.y >= HEIGHT) { star.y = 0; star.x = rand() % WIDTH; }
    }

    for (auto it = activeParticles.begin(); it != activeParticles.end(); ) {
        (*it)->x += (*it)->vx; (*it)->y += (*it)->vy; (*it)->life--;
        if ((*it)->life <= 0) { particlePool.deallocate(*it); it = activeParticles.erase(it); }
        else ++it;
    }

    for (auto it = activeBullets.begin(); it != activeBullets.end(); ) {
        (*it)->y--;
        if ((*it)->y < 0) { bulletPool.deallocate(*it); it = activeBullets.erase(it); }
        else ++it;
    }

    if (rand() % (10 / (level > 5 ? 5 : level) + 1) == 0) {
        Enemy* e = enemyPool.allocate();
        e->x = rand() % WIDTH; e->y = 0;
        if (rand() % 10 < level) {
            e->type = 1; e->hp = 3;
        } else {
            e->type = 0; e->hp = 1;
        }
        activeEnemies.push_back(e);
    }

    for (auto it = activeEnemies.begin(); it != activeEnemies.end(); ) {
        (*it)->y++;
        if ((*it)->y >= HEIGHT) { enemyPool.deallocate(*it); it = activeEnemies.erase(it); }
        else if (abs((*it)->x - playerX) < 1 && abs((*it)->y - playerY) < 1) { gameOver = true; break; }
        else ++it;
    }

    for (auto itB = activeBullets.begin(); itB != activeBullets.end(); ) {
        bool b_destroyed = false;
        for (auto itE = activeEnemies.begin(); itE != activeEnemies.end(); ) {
            if ((*itB)->x == (*itE)->x && (*itB)->y == (*itE)->y) {
                (*itE)->hp--;
                if ((*itE)->hp <= 0) {
                    score += ((*itE)->type == 1 ? 500 : 100);
                    CreateExplosion((*itE)->x, (*itE)->y, (*itE)->type == 1 ? 2 : 1);
                    enemyPool.deallocate(*itE); itE = activeEnemies.erase(itE);
                }
                b_destroyed = true; break;
            } else ++itE;
        }
        if (b_destroyed) { bulletPool.deallocate(*itB); itB = activeBullets.erase(itB); }
        else ++itB;
    }
}
