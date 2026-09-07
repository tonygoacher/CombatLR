#include <Arduino.h>
#include "statemachine/GameInitState.h"
#include "graphics/DisplayManager.h"
#include "graphics/DisplayBackgroundCommand.h"
#include "IZoom.h"
#include "Navigation.h"
#include "graphics/PlayerPositionCommand.h"
#include "graphics/TurnMapToHeadingCommand.h"
#include "ZoneEngine.h"
#include "statemachine/SystemStates.h"
#include "graphics/ZoomCommand.h"
#include "widgets/UIWidgetIds.h"
#include "graphics/AddWidgetCommand.h"
#include "graphics/RemoveWidgetCommand.h"
#include "memory.h"
#include "graphics/SetTextCommand.h"
#include "graphics/EnableWidgetCommand.h"
#include "ISounds.h"
#include <format>




GameInitState::GameInitState(uint32_t timeToGameStart,CombatLRServices& stateData)
:
 _stateData(stateData), _timeToGameStart(timeToGameStart),  _textPacer(true)
{
    Serial.println("GameInitState::GameInitState");
    _textPacer.SetPace(1000);
}



void GameInitState::Entry()
{
    Serial.println("GameInitState::Entry");
    _stateData.displayManager.Send(
    std::unique_ptr<IDisplayCommand>(
        new DisplayBackgroundCommand(
            "/map.bmp")));
     
    _stateData.zoom.Reset();
     Serial.println("!!GameInitState::Entry");


     WidgetPosition textPos={TextX,TextY};
     _textWidget  = new TextWidget(UIWidgetIds::CountdownTimer, "", textPos,TextFont,MC_DATUM, TFT_RED, TFT_BLACK, TextZoom);
     


     
     _textWidget->SetPosition(textPos);
    _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(
                    _textWidget)));

    _textPacer.SetPace(1000);   
             
}




uint16_t GameInitState::Run()
{

    if(_textPacer.Pace())
    {

        if(_timeToGameStart == 0)
        {
            return SystemStates::GameRunning;
        }

        char newText[64];

        sprintf(newText,"%02d:%02d", _timeToGameStart/60 , _timeToGameStart%60);

        _stateData.displayManager.Send(std::unique_ptr<IDisplayCommand>(new SetTextCommand(UIWidgetIds::CountdownTimer,newText)));
        
            
        if(_timeToGameStart == 60)
        {
            Serial.println("T60");
            _stateData.sounds.Play("/sounds/t60.wav");
        }

        if(_timeToGameStart == 10)
        {
            Serial.println("T10");
            _stateData.sounds.Play("/sounds/t10.wav");
        }

        if(_timeToGameStart <= 5 && _timeToGameStart)
        {
       //     _stateData.sounds.Play("/sounds/countd.wav");
        }

        if(_timeToGameStart)
        {
            _timeToGameStart--;
        }  

    }


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
                    

    return SystemStates::GameInit;  
}


void GameInitState::Exit()
{
     _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new RemoveWidgetCommand(
                    _textWidget->GetId())));

}
