#pragma once

#include <stdint.h>
#include "GameCoordinate.h"
#include "Pacer.h"

class ISounds;


class Player
{
public:
    Player(ISounds& sounds);
    GameCoordinate& Position();
    void InitHealth(int startHealth = 100, int maxHealth = 100);
    int GetHealth();        // Player health = 0 - 100
    unsigned int AddHealth(unsigned int health);  // Capped to 100
    unsigned int TakeHealth(unsigned int health); // Capped to 0
    void SetPosition(GameCoordinate& newPos);
    bool AtMaxHealth();
    bool CheckInZone(bool inZone);
    int GetMaxHealth() { return _maxHealth;}

private:
    static const int DefaultMaxHealth = 100;
    int _maxHealth;
    GameCoordinate  _playerPos;
    int         _health;
    ISounds&        _sounds;
    bool _outOfZone = false;
    Pacer   _outOfZonePacer;
    uint8_t _outOfZoneHealthDecrement;
    uint8_t _outOfZoneCount;

};