#include <Arduino.h>
#include "statemachine/GameInitState.h"
#include "DisplayManager.h"
#include "DisplayBackgroundCommand.h"
#include "IZoom.h"
#include "Navigation.h"
#include "PlayerPositionCommand.h"
#include "TurnMapToHeadingCommand.h"
#include "ZoneEngine.h"
#include "statemachine/SystemStates.h"
#include "ZoomCommand.h"
#include "widgets/UIWidgetIds.h"
#include "AddWidgetCommand.h"
#include "RemoveWidgetCommand.h"
#include "memory.h"
#include "SetTextCommand.h"
#include "EnableWidgetCommand.h"
#include <format>




GameInitState::GameInitState(uint32_t timeToGameStart,StateData& stateData)
:
 _stateData(stateData), _timeToGameStart(timeToGameStart), _pacer(false), _textPacer(true)
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
    _pacer.SetPace(_timeToGameStart * 1000);         
    _stateData.zoom.Reset();
     Serial.println("!!GameInitState::Entry");


     WidgetPosition textPos={280,10};
     _textWidget  = new TextWidget(UIWidgetIds::CountdownTimer, "Test", textPos,2,MC_DATUM, TFT_RED, TFT_BLACK, 2);
     


     
     _textWidget->SetPosition(textPos);
    _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(
                    _textWidget)));

    _textPacer.SetPace(1000);   
             
}




uint16_t GameInitState::Run()
{
    static int a=300;
    if(_textPacer.Pace())
    {
        if(a != 0)
        {
            a--;
        }

        char newText[64];
        sprintf(newText,"%02d:%02d", a/60 , a%60);

        _stateData.displayManager.Send(std::unique_ptr<IDisplayCommand>(new SetTextCommand(UIWidgetIds::CountdownTimer,newText)));


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
