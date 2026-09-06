#pragma once
#include "object_pool.h"
#include <list>
#include <vector>

const int WIDTH = 40;
const int HEIGHT = 25;

struct Bullet {
    int x, y;
    Bullet() : x(0), y(0) {}
};

struct Enemy {
    int x, y;
    int hp;
    int type;
    Enemy() : x(0), y(0), hp(1), type(0) {}
};

struct Particle {
    float x, y;
    float vx, vy;
    int life;
    Particle() : x(0), y(0), vx(0), vy(0), life(0) {}
};

struct Star {
    int x, y;
    int speed;
};

extern int playerX, playerY;
extern int score;
extern bool gameOver;
extern int level;

extern ObjectPool<Bullet> bulletPool;
extern ObjectPool<Enemy> enemyPool;
extern ObjectPool<Particle> particlePool;

extern std::list<Bullet*> activeBullets;
extern std::list<Enemy*> activeEnemies;
extern std::list<Particle*> activeParticles;
extern std::vector<Star> backgroundStars;
