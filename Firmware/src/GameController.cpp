#include "GameController.h"
#include "IGameTime.h"
GameController& GameController::Instance()
{
    static GameController instance;
    return instance;
}

GameController::GameController()
    : _gamePlan(nullptr),
      _gameTime(nullptr)
{
}


void GameController::Initialise(
    const GamePlan& gamePlan,
    IGameTime& gameTime)
{
    Serial.println("GameController initialised");
    _gamePlan = &gamePlan;
    _gameTime = &gameTime;
}

void GameController::ShowZoneInfo()
{
    Serial.print("There are "); Serial.print(_gamePlan->GetZones().size()); Serial.println(" zones.");
    for(int i = 0 ; i < _gamePlan->GetZones().size(); i++)
    {
        Serial.print("Zone "); Serial.println(i);
        Serial.print("Start time  :");Serial.println(_gamePlan->GetZones()[i].startTime);
        Serial.print("Lifespan    :");Serial.println(_gamePlan->GetZones()[i].lifespanSeconds);

        std::map< int, Pickup>::const_iterator pickupIter = _gamePlan->GetZones()[i].pickups.cbegin();
        while(pickupIter != _gamePlan->GetZones()[i].pickups.cend())
        {
            Serial.print("Pickup ID   :"); Serial.println(pickupIter->second.id);
            Serial.print("Pickup Type :"); Serial.println(pickupIter->second.type);
            Serial.print("Start time  :"); Serial.println(pickupIter->second.spawnTimeSeconds);
            pickupIter++;
        }            
    }
}