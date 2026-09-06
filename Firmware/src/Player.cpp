#include <Arduino.h>
#include "Player.h"
#include "ISounds.h"




namespace PlayerStrings
{
    static const char * const addingHealth="/sounds/addhlth.wav";
}

Player::Player(ISounds& sounds, int initialHealth)
: _sounds(sounds), _health(initialHealth)
{
    
}

GameCoordinate& Player::Position()
{
    return _playerPos;    
}

int Player::GetHealth()
{
    return _health;  
}

unsigned int Player::AddHealth(unsigned int addHealth)
{
    _sounds.Play(PlayerStrings::addingHealth); 
    _health += addHealth;
    _health = min(_health, 100);
    return _health;
}

unsigned int Player::TakeHealth(unsigned int takeHealth)
{
    _health -= takeHealth;

    _health = max(_health,0);
    return _health;
}

void Player::SetPosition(GameCoordinate& newPos)
{
    _playerPos = newPos;
}
