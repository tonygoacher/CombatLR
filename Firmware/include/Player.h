#pragma once

#include <stdint.h>
#include "GameCoordinate.h"

class ISounds;

class Player
{
public:
    Player(ISounds& sounds, int initialHealth);
    GameCoordinate& Position();
    int GetHealth();        // Player health = 0 - 100
    unsigned int AddHealth(unsigned int health);  // Capped to 100
    unsigned int TakeHealth(unsigned int health); // Capped to 0
    void SetPosition(GameCoordinate& newPos);


private:
    GameCoordinate  _playerPos;
    int         _health;
    ISounds&        _sounds;

};