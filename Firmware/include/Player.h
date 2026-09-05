#pragma once

#include <stdint.h>
#include "GameCoordinate.h"

class ISounds;

class Player
{
public:
    Player(ISounds& sounds, int initialHealth);
    GameCoordinate& Position();
    int GetHealth();
    int AddHealth(int health);
    int TakeHealth(int health);
    void SetPosition(GameCoordinate& newPos);


private:
    GameCoordinate  _playerPos;
    int         _health;
    ISounds&        _sounds;

};