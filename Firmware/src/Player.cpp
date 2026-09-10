#include <Arduino.h>
#include "Player.h"
#include "ISounds.h"
#include "Pacer.h"




namespace PlayerStrings
{
    static const char * const addingHealth="/sounds/addhlth.wav";
}

Player::Player(ISounds& sounds, int initialHealth)
: _sounds(sounds), _health(initialHealth), _outOfZonePacer(true), _outOfZoneHealthDecrement(1), _outOfZoneCount(0)
{
    
}


void Player::InitHealth()
{
    _health = MaxHealth;
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
    _health = min(_health, MaxHealth);
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
    return _health >= MaxHealth;
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
        Serial.println("Out of zone");
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
