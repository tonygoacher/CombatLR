#pragma once

#include <stdint.h>
#include "GameCoordinate.h"
#include "Pacer.h"

class ISounds;


class Player
{
public:
    Player(ISounds& sounds, int initialHealth);
    GameCoordinate& Position();
    void InitHealth();
    int GetHealth();        // Player health = 0 - 100
    unsigned int AddHealth(unsigned int health);  // Capped to 100
    unsigned int TakeHealth(unsigned int health); // Capped to 0
    void SetPosition(GameCoordinate& newPos);
    bool AtMaxHealth();
    bool CheckInZone(bool inZone);

private:
    static const int MaxHealth = 50;
    GameCoordinate  _playerPos;
    int         _health;
    ISounds&        _sounds;
    bool _outOfZone = false;
    Pacer   _outOfZonePacer;
    uint8_t _outOfZoneHealthDecrement;
    uint8_t _outOfZoneCount;

};