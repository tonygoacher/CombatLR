
#include <vector>

#include "stateMachine/GameRunningState.h"
#include "ISystemTime.h"
#include "DisplayManager.h"
#include "Navigation.h"
#include "GameCoordinate.h"
#include "PlayerPositionCommand.h"
#include "TurnMapToHeadingCommand.h"
#include "ZoomCommand.h"
#include "stateMachine/SystemStates.h"
#include "IZoom.h"
#include "PBSwitch.h"
#include "ISounds.h"

#include "weapons/AssaultRifle.h"
#include "weapons/WeaponManager.h"

#include "widgets/PlayerHealthWidget.h"
#include "AddWidgetCommand.h"
#include "RemoveWidgetCommand.h"
#include "widgets/UIWidgetIds.h"




GameRunningState::GameRunningState(uint16_t gameTime /*secs*/, CombatLRServices& stateData, WeaponManager& weaponManager)
: _stateData(stateData), _gameTime(gameTime), _player(stateData.sounds, PlayerStartHealth), _weaponManager(weaponManager), _ammoPickup(weaponManager)
{
}


uint16_t GameRunningState::Run()
{

    
    if(_stateData.navigation.Update())
    {
        
        NavigationData data = _stateData.navigation.GetData();

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

    _stateData.zoneEngine.Update(0);


   _weaponManager.Run();

    CheckPickups();


    return SystemStates::GameRunning;  

}



void GameRunningState::Entry()
{
    _weaponManager.Init();
    _stateData.systemTime.Reset();

    _weaponManager.AddWeapon(WeaponType::AssaultRifle, 120);
    _weaponManager.AddWeapon(WeaponType::AssaultRifleFullAuto, 120);

    PlayerHealthWidget*  healthDisplay = new PlayerHealthWidget(UIWidgetIds::PlayerHealth, _player, HealthX, HealthY, HealthW, HealthH);

    _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(healthDisplay)));


}




void GameRunningState::Exit()
{

    _stateData.displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new RemoveWidgetCommand(UIWidgetIds::PlayerHealth)));
}



void GameRunningState::CheckPickups()
{
    std::vector<ZoneEngine::PickupInfo> nearbyPickups;

  //  Serial.printf("Player E: %.3f N: %.3f\n", _playerPosition.east, _playerPosition.north);
    if(_stateData.zoneEngine.GetPickupsWithin(_player.Position(), 5.0f,nearbyPickups))
    {
        Serial.println("Nearby");
        for( auto item : nearbyPickups)
        {
            switch(item.type)
            {
                case   PickupTypes::Ammo:
                {
                    PickupAmmo(item);
                }
                break;

                case    PickupTypes::Health:
                {
                    PickupHealth(item);
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


