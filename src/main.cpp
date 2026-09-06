#include "game_core.h"
#include "benchmark.h"
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <windows.h>

using namespace std;

void run_game_mode() {
    system("chcp 65001");
    srand((unsigned)time(0));
    InitGame();

    for (int i = 0; i < 20; i++) bulletPool.deallocate(new Bullet());
    for (int i = 0; i < 20; i++) enemyPool.deallocate(new Enemy());
    for (int i = 0; i < 150; i++) particlePool.deallocate(new Particle());

    while (!gameOver) {
        Draw();
        Input();
        Logic();
        Sleep(40);
    }

    system("cls");
    cout << "\033[91m";
    cout << "=======================\n";
    cout << "      GAME OVER        \n";
    cout << "=======================\n\033[0m";
    cout << "Final Score: " << score << endl;
    
    cout << "\n[Technical Summary]" << endl;
    cout << "Total system new calls intercepted: " 
         << bulletPool.total_count() + enemyPool.total_count() + particlePool.total_count() 
         << " times!" << endl;
    
    system("pause");
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        run_all_benchmarks();
    } else {
        run_game_mode();
    }
    return 0;
}
