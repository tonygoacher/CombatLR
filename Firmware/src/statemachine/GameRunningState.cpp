
#include <vector>

#include "stateMachine/GameRunningState.h"
#include "ISystemTime.h"
#include "graphics/DisplayManager.h"
#include "Navigation.h"
#include "GameCoordinate.h"
#include "graphics/PlayerPositionCommand.h"
#include "graphics/TurnMapToHeadingCommand.h"
#include "graphics/ZoomCommand.h"
#include "stateMachine/SystemStates.h"
#include "IZoom.h"
#include "PBSwitch.h"
#include "ISounds.h"

#include "weapons/AssaultRifle.h"
#include "weapons/WeaponManager.h"

#include "widgets/PlayerHealthWidget.h"
#include "graphics/AddWidgetCommand.h"
#include "graphics/RemoveWidgetCommand.h"
#include "widgets/UIWidgetIds.h"
#include "ESP32Lasertag.h"




GameRunningState::GameRunningState(uint16_t gameTime /*secs*/, CombatLRServices& stateData, WeaponManager& weaponManager)
: _stateData(stateData), _gameTime(gameTime), _player(stateData.sounds, PlayerStartHealth),
 _weaponManager(weaponManager), 
 _ammoPickup(weaponManager),
 _weaponPickupTable("/wtable.json"),
 _startTime(0),
 _weaponPickup(weaponManager, stateData.zoneEngine, _weaponPickupTable)
{
}


uint16_t GameRunningState::Run()
{
    LaserTagData ltData;
    if(_stateData.laserTag.GetLasertagData(ltData))
    {
        Serial.println("Got LT data");
        Serial.printf("Shot weapon: %d\n", ltData.weapon);

  
        int shotPower = _stateData.weaponConfig.GetShotPower(ltData.weapon);
        Serial.printf("Shot weapon %d\n", ltData.weapon);
        Serial.printf("Shot power %d\n", shotPower);
        if(shotPower)
        {
            int newHealth = _player.TakeHealth(shotPower);
            if(newHealth)
            {
                _stateData.sounds.Play("/sounds/hit.wav");  
            }
            else
            {
                Serial.println("DEAD");
            }
        }
    }

    
    if(_stateData.navigation.Update())
    {
        
        NavigationData data = _stateData.navigation.GetData();
    //    data.latitude =  53.5509289;
     //   data.longitude = -1.9987657;

      //  data.latitude =  53.5515236;
    //    data.longitude = -1.9982851;

        /*
        * Convert the known C GPS position into the local
        * GameCoordinate system.
        */

        GameCoordinate 
        playerPosition =
            _stateData.geoReference.ToGameCoordinate(
                data.latitude,
                data.longitude);
         
        _player.SetPosition(playerPosition);    
        
               
        _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new PlayerPositionCommand(
                    _player.Position())));

                           

        _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new TurnMapToHeadingCommand(
                    data.heading)));

                
    }   
    
    float zoom = 1.0f;
    if(_stateData.zoom.GetZoom(zoom))   // Only process zoom if it has changed
    {
        Serial.println("Zoom");
        _stateData.sounds.Play("/sounds/zoom.wav");
        _stateData.displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new ZoomCommand(
                zoom)));    
    }

    uint32_t elapsedTime = uint32_t(_stateData.navigation.GetGpsTime() - _startTime);
    _stateData.zoneEngine.Update(elapsedTime);


   _weaponManager.Run();

    CheckPickups();

  //  if(_player.CheckInZone(_stateData.zoneEngine.InZone(_player.Position())))
    {
    //    _stateData.sounds.Play("/sounds/hit.wav");
    }

    

    return SystemStates::GameRunning;  

}



void GameRunningState::Entry()
{
    _weaponManager.Init();
    _stateData.systemTime.Reset();
    _player.InitHealth();
    _stateData.laserTag.Init();
    PlayerHealthWidget*  healthDisplay = new PlayerHealthWidget(UIWidgetIds::PlayerHealth, _player, HealthX, HealthY, HealthW, HealthH);

    _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(healthDisplay)));

    _startTime = _stateData.navigation.GetGpsTime();            
    
    _stateData.zoneEngine.AddZoneEventListener(this);

    _weaponManager.AddWeapon(5);

}




void GameRunningState::Exit()
{

    _stateData.displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new RemoveWidgetCommand(UIWidgetIds::PlayerHealth)));

    _stateData.zoneEngine.RemoveZoneEventListener(this);    
    _stateData.zoneEngine.RemoveObjectsInCurrentZone();

    _stateData.displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new RemoveWidgetCommand(UIWidgetIds::AmmoDisplay)));

    _stateData.displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new RemoveWidgetCommand(UIWidgetIds::SelectWeapon)));
}



void GameRunningState::CheckPickups()
{
    std::vector<ZoneEngine::PickupInfo> nearbyPickups;

  //  Serial.printf("Player E: %.3f N: %.3f\n", _playerPosition.east, _playerPosition.north);
    if(_stateData.zoneEngine.GetPickupsWithin(_player.Position(), 2.0f,nearbyPickups))
    {
        Serial.printf("Nearby %d\n", nearbyPickups.size());
        for( auto item : nearbyPickups)
        {
            Serial.printf("Pickup type %d ID:%d \n", item.type, item.id);
            switch(item.type)
            {
                case   PickupTypes::Ammo:
                {
                    PickupAmmo(item);
                }
                break;

                case    PickupTypes::Health:
                {
                    if(!_player.AtMaxHealth())  // Pickup health only if not already full.
                    {
                        PickupHealth(item);
                    }                        
                }
                break;

                case    PickupTypes::WeaponBonus:
                {
                    PickupBonus(item);
                }
                break;


            }   
        }
    }    
}





void GameRunningState::PickupBonus(ZoneEngine::PickupInfo &pickup)
{
    Serial.println("Found a weapon pickup");
    _stateData.zoneEngine.ClaimPickup(pickup.id);
    _weaponPickup.Collect();
}


void GameRunningState::PickupHealth(ZoneEngine::PickupInfo &pickup)
{
    Serial.println("Health");
    _stateData.zoneEngine.ClaimPickup(pickup.id);
    _player.AddHealth(random(50,100));
}


void GameRunningState::PickupAmmo(ZoneEngine::PickupInfo &pickup)
{
    _ammoPickup.Collect();
    _stateData.zoneEngine.ClaimPickup(pickup.id);  
 
}





void GameRunningState::ZoneMoveImminent(GameCoordinate newZonePos,int zoneIndexInGamePlan,uint32_t timeToActivation)
{
    _stateData.sounds.Play("/sounds/20swarn.wav");
}



void GameRunningState::ZoneStartedMoving(GameCoordinate newZonePos, int zoneIndexInGamePlan)
{
    _stateData.sounds.Play("/sounds/zmove.wav");
}



void GameRunningState::NewZoneActive(GameCoordinate newZonePos,int zoneIndexInGamePlan)
{
    _stateData.sounds.Play("/sounds/zactive.wav");
}



void GameRunningState::PickupActivated(uint32_t pickupId)
{
    Serial.printf("Pickup activated %d\n", pickupId);
}


void GameRunningState::PickupDeactivated(uint32_t pickupId)
{
    Serial.printf("Pickup deactivated %d\n", pickupId);
}
