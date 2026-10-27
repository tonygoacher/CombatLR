#include <Arduino.h>
#include "statemachine/GameInitState.h"
#include "statemachine/CombatLRStateMachine.h"
#include "statemachine/GenericEvent.h"
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
#include "GamePlan.h"
#include "SystemTime.h"
#include <format>




GameInitState::GameInitState(CombatLRServices& stateData)
:
 _services(stateData)
{
    Serial.println("GameInitState::GameInitState");
}



void GameInitState::SetCountdown()
{

    GenericEvent  countdownTimer(GameInitState::Countdown);
    _services.stateMachine.ScheduleEvent(countdownTimer, oneSecondsInms);
}


void GameInitState::Entry()
{
    Serial.println("GameInitState::Entry");
    _services.displayManager.Send(
    std::unique_ptr<IDisplayCommand>(
        new DisplayBackgroundCommand(
            "/map.bmp")));
     
    _services.zoom.Reset();
     Serial.println("!!GameInitState::Entry");


     WidgetPosition textPos={TextX,TextY};
     _textWidget  = new TextWidget(UIWidgetIds::CountdownTimer, "", textPos,TextFont,MC_DATUM, TFT_RED, TFT_BLACK, TextZoom);
     


     
     _textWidget->SetPosition(textPos);
    _services.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(
                    _textWidget)));

    SetCountdown();
}




uint16_t GameInitState::Run()
{
    _services.systemTime.Update();
    
    if(_services.systemTime.GetSeconds() >= _services.gamePlan.GetGameDelayTime())
    {
        return SystemStates::GameRunning;
    }

    auto event = _services.stateMachine.GetEvent();
    if(event && event->GetType() == GameInitState::Countdown)
    {
        SetCountdown();

        Serial.printf("GameInitState T:%d\n", (int)_services.systemTime.GetSeconds());

        uint32_t timeToGameStart = _services.gamePlan.GetGameDelayTime() - _services.systemTime.GetSeconds();
 
        char newText[64];

        sprintf(newText,"%02d:%02d", timeToGameStart/60 , timeToGameStart%60);

        _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(new SetTextCommand(UIWidgetIds::CountdownTimer,newText)));
        
            
        if(timeToGameStart == 60)
        {
            Serial.println("T60");
            _services.sounds.Play("/sounds/t60.wav");
        }

        if(timeToGameStart == 10)
        {
            Serial.println("T10");
            _services.sounds.Play("/sounds/t10.wav");
        }

        if(timeToGameStart <= 5 && timeToGameStart)
        {
            _services.sounds.Play("/sounds/countd.wav");
        }
    }


    if(_services.navigation.Update())
    {
        
        NavigationData data = _services.navigation.GetData();



        /*
        * Convert the known C GPS position into the local
        * GameCoordinate system.
        */
        _playerPosition =
            _services.geoReference.ToGameCoordinate(
                data.latitude,
                data.longitude);
             
        
               
        _services.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new PlayerPositionCommand(
                    _playerPosition)));

        _services.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new TurnMapToHeadingCommand(
                    data.heading)));

        float zoom = 1.0f;
        if(_services.zoom.GetZoom(zoom))
        {
            Serial.println(zoom);
            _services.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new ZoomCommand(
                    zoom)));    
        }       
    }       

    _services.zoneEngine.Update(0);
                    

    return SystemStates::GameInit;  
}


void GameInitState::Exit()
{
    _services.stateMachine.CancelAllEvents();
    
     _services.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new RemoveWidgetCommand(
                    _textWidget->GetId())));

}
