
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





GameRunningState::GameRunningState(uint16_t gameTime /*secs*/, StateData& stateData)
: _stateData(stateData), _gameTime(gameTime), _playerPosition(0,0)
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

        float zoom = 1.0f;
        if(_stateData.zoom.GetZoom(zoom))
        {
            Serial.println(zoom);
            _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new ZoomCommand(
                    zoom)));    
        }

                
    }       

    _stateData.zoneEngine.Update(0);

    if(_stateData.triggerSwitch.Pressed())
    {
        Serial.println("Trigger pressed");
    }

    if(_stateData.reloadSwitch.Pressed())
    {
        Serial.println("Reload pressed");
    }


    if(_stateData.leftSwitch.Pressed())
    {
        Serial.println("Left pressed");
    }

        if(_stateData.zoomSwitch.Pressed())
    {
        Serial.println("Zoom pressed");
    }

        if(_stateData.rightSwitch.Pressed())
    {
        Serial.println("Right pressed");
    }
                    

    return SystemStates::GameRunning;  

}



void GameRunningState::Entry()
{
    _stateData.systemTime.Reset();

}




void GameRunningState::Exit()
{

}
