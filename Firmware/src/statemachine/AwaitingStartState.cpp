#include "WiFi.h"
#include "WiFiUdp.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "ISounds.h"
#include "statemachine/SystemStates.h"
#include "stateMachine/AwaitingStartState.h"
#include "graphics/DisplayManager.h"
#include "graphics/DisplayBackgroundCommand.h"
#include "statemachine/services.h"
#include "widgets/WiFiStatusWidget.h"
#include "widgets/UIWidgetIds.h"
#include "graphics/AddWidgetCommand.h"
#include "graphics/RemoveWidgetCommand.h"
#include "graphics/SetTextureCommand.h"
#include "graphics/SetTextCommand.h"
#include "widgets/TextureWidget.h"
#include "TeamIDs.h"
#include "widgets/TextWidget.h"
#include "SystemTime.h"
#include <SimpleFTPServer.h>
#include "GamePlan.h"





namespace TeamIconFiles
{
    const char * const redTeam = "/teamred.bmp";
    const char * const blueTeam = "/teamblue.bmp";
    const char * const yellowTeam = "/teamyellow.bmp";
    const char * const greenTeam = "/teamgreen.bmp";
}



const char * const AwaitingStartState::GetTeamFile(TeamIds teamId)
{
    switch(teamId)
    {
        case    TeamIds::Red:
        {
            return TeamIconFiles::redTeam;
        }
        break;

        case    TeamIds::Blue:
        {
            return TeamIconFiles::blueTeam;
        }
        break;

        case    TeamIds::Yellow:
        {
            return TeamIconFiles::yellowTeam;
        }
        break;

        case    TeamIds::Green:
        {
            return TeamIconFiles::greenTeam;
        }
        break;

        default:
        {
            Serial.printf("AwaitingStartState::GetTeamFile Invalid team id %d\n", static_cast<int>(teamId));
        }

    }
    return nullptr;
}


// -----------------------------------------------------------------------------
// AwaitingStartState
// -----------------------------------------------------------------------------

AwaitingStartState::AwaitingStartState(
    CombatLRServices& services)
    : _services(services),
      _connectionShown(false),
      _network(services.systemConfig, services.gamePlan.GetVersion(), services.gamePlan.GetProjectName()),
     _exitPacer(false),
     _networkShudown(false)
{
    _exitPacer.SetPace(1000);   // USed to prevent early shutdown of wifi before ACK has been sent
}


void AwaitingStartState::EntryCommon()
{
    _services.displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new DisplayBackgroundCommand(
                "/logo.bmp")));
    WidgetPosition posWifi(320-32,0);

    WiFiStatusWidget* statusWidget = new WiFiStatusWidget(UIWidgetIds::WiFiStatus,
         _services.displayManager.GetTextureManager(),posWifi);

    _services.displayManager.Send(std::make_unique<AddWidgetCommand>(statusWidget));   

    WidgetPosition textPos={TextX,TextY};

    TextWidget*  textWidget  = new TextWidget(UIWidgetIds::IPAddress, WiFi.macAddress().c_str(), textPos,TextFont,MC_DATUM, TFT_WHITE, TFT_BLACK, TextZoom);
    
    _services.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(
                    textWidget)));    
    
    WiFi.mode(
        WIFI_STA);

    WiFi.begin(
        _services.systemConfig.GetWifiSSID().c_str(),
        _services.systemConfig.GetWifiPass().c_str());

    Serial.println(
        "Connecting to Wi-Fi");

    _connectionShown =
        false;                    
    
}


// -----------------------------------------------------------------------------
// Entry
// -----------------------------------------------------------------------------

void AwaitingStartState::Entry()
{

    EntryCommon();
    _network.IndicateFtpMode(false);

    TextureWidget* textureWidget = nullptr;
    WidgetPosition posTeam(320-64,0);

    TeamIds team = static_cast<TeamIds>(_services.systemConfig.GetTeamId());
    textureWidget  = new TextureWidget(UIWidgetIds::TeamID,
        _services.displayManager.GetTextureManager(),posTeam,GetTeamFile(team));
    _services.displayManager.Send(std::make_unique<AddWidgetCommand>(textureWidget));               

}

// -----------------------------------------------------------------------------
// Exit
// -----------------------------------------------------------------------------


void AwaitingStartState::Exit()
{


    RemoveWidgetCommand* removeIcon = new RemoveWidgetCommand(UIWidgetIds::WiFiStatus);

    _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(removeIcon));      

    removeIcon = new RemoveWidgetCommand(UIWidgetIds::TeamID);

    _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(removeIcon));  

    removeIcon = new RemoveWidgetCommand(UIWidgetIds::IPAddress);

    _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(removeIcon));  
    
 
    _network.Stop();

    WiFi.disconnect(
        true);

    WiFi.mode(
        WIFI_OFF);
}

// -----------------------------------------------------------------------------
// Run
// -----------------------------------------------------------------------------

void AwaitingStartState::RunCommon()
{

    if (!_connectionShown)
    {
        Serial.println();
        Serial.println(
            "Connected");

        Serial.print(
            "IP address: ");
        Serial.println(
            WiFi.localIP());

        Serial.print(
            "MAC address: ");
        Serial.println(
            WiFi.macAddress());


        _services.displayManager.Send(
            std::make_unique<SetTextCommand>(UIWidgetIds::IPAddress, WiFi.localIP().toString().c_str()));

        _network.Begin();

        _connectionShown =
            true;
    }
    

}

uint16_t AwaitingStartState::Run()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return SystemStates::AwaitingStart;
    }

    RunCommon();

  
    if(_networkShudown)
    {

        if(_exitPacer.Pace())
        {
            Serial.println("Closing");
            return SystemStates::GameInit;   
        }
        Serial.println("Awaiting network clear");
        return SystemStates::AwaitingStart;   
    }
    



    /*
     * UDP command handling is deliberately pumped from the
     * existing AwaitingStartState loop. There is no HTTP server
     * and no separate task.
     */

    std::unique_ptr<CombatLRNetworkEvent> event =  _network.Update();

    if(event != nullptr)
    {
        if(event->GetType() == CombatLRNetworkEventType::TeamChanged)
        {
            TeamIds team = static_cast<TeamIds>(_services.systemConfig.GetTeamId());
            _services.displayManager.Send(
                std::make_unique<SetWidgetTextureCommand>(
                UIWidgetIds::TeamID,
                GetTeamFile(team))); 
        }

        if(event->GetType() == CombatLRNetworkEventType::StartGame)
        {
            auto* startCommand =
                static_cast<StartGameCommand*>(event.get());

            if(_services.systemTime.Reset(startCommand->GetGameStart().c_str()))
            {
                _networkShudown = true;
                _exitPacer.PacerReset();
            }           
        }
      
    }        
    return SystemStates::AwaitingStart;
}




