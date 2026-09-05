
#include "stateMachine/GameRunningState.h"
#include "ISystemTime.h"
#include "DisplayManager.h"
#include "Navigation.h"
#include "GameCoordinate.h"
#include "PlayerPositionCommand.h"
#include "TurnMapToHeadingCommand.h"
#include "ZoomCommand.h"
#include "ZoneEngine.h"
#include "stateMachine/SystemStates.h"
#include "IZoom.h"
#include "PBSwitch.h"
#include "ISounds.h"

#include "weapons/AssaultRifle.h"
#include "weapons/WeaponManager.h"



GameRunningState::GameRunningState(uint16_t gameTime /*secs*/, CombatLRServices& stateData, WeaponManager& weaponManager)
: _stateData(stateData), _gameTime(gameTime), _playerPosition(0,0), _weaponManager(weaponManager)
{
}

int a=0;

uint16_t GameRunningState::Run()
{

    
    if(_stateData.navigation.Update())
    {
        
        NavigationData data = _stateData.navigation.GetData();

        /*
        * Convert the known C GPS position into the local
        * GameCoordinate system.
        */
        _playerPosition =
            _stateData.geoReference.ToGameCoordinate(
                data.latitude,
                data.longitude);
             
        
               
        _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new PlayerPositionCommand(
                    _playerPosition)));

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


    return SystemStates::GameRunning;  

}



void GameRunningState::Entry()
{
    _weaponManager.Init();
    _stateData.systemTime.Reset();
    Serial.println("Adding weapon");
    _weaponManager.AddWeapon(WeaponType::AssaultRifle, 120);
    _weaponManager.AddWeapon(WeaponType::AssaultRifleFullAuto, 120);
    Serial.println("Added");
}




void GameRunningState::Exit()
{

}
