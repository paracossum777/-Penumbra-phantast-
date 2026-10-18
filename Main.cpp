#include <iostream>
#include "Game.h"
#include "Battle.h"

using namespace sf;

int main()
{
    // Игровой движок
    Game game;

    // Игровой цикл окна
    while (game.getWindowisOpen())
    {
        // Обновление логики
        game.update();

        // Отрисовка кадра
        game.render();
    }

    return 0;
}
