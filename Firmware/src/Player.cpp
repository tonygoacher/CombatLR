#include <Arduino.h>
#include "Player.h"
#include "ISounds.h"
#include "Pacer.h"




namespace PlayerStrings
{
    static const char * const addingHealth="/sounds/addhlth.wav";
}

Player::Player(ISounds& sounds)
: _sounds(sounds),  _outOfZonePacer(true), _outOfZoneHealthDecrement(1), _outOfZoneCount(0)
{
    _maxHealth = DefaultMaxHealth ;
    _health = _maxHealth;
}


void Player::InitHealth(int startHealth , int maxHealth )
{
    _maxHealth = maxHealth;
    _health = startHealth;

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
    _health = min(_health, DefaultMaxHealth);
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



bool Player::AtMaxHealth()
{
    return _health >= DefaultMaxHealth;
}




bool Player::CheckInZone(bool inZone)
{
    bool healthTaken = false;
    if(inZone  || _health == 0)
    {

        _outOfZoneHealthDecrement = 1;
        _outOfZonePacer.SetPace(1000);
  
    }
    else
    {
        if(_outOfZonePacer.Pace())
        {
            TakeHealth(_outOfZoneHealthDecrement);
            healthTaken = true;

            _outOfZoneCount++;
            if(_outOfZoneCount > 3 && _outOfZoneCount < 8)
            {
                _outOfZoneHealthDecrement = 10;
            }
            if(_outOfZoneCount >=8)
            {
                _outOfZoneHealthDecrement = 25;
            }
        }        
    }
    return healthTaken;
}
