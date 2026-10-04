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


AwaitingStartState* AwaitingStartState::_ftpCallbackInstance = nullptr;


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
     _networkShudown(false),
     _ftpInitDone(false),
     _ftpServer(nullptr)
{
    _exitPacer.SetPace(1000);   // USed to prevent early shutdown of wifi before ACK has been sent
}

// -----------------------------------------------------------------------------
// Entry
// -----------------------------------------------------------------------------

void AwaitingStartState::Entry()
{
    if (FtpMode())
    {
        _ftpCallbackInstance = this;
    }

    _services.displayManager.Send(
        std::unique_ptr<IDisplayCommand>(
            new DisplayBackgroundCommand(
                "/logo.bmp")));

    WidgetPosition posWifi(320-32,0);

    WiFiStatusWidget* statusWidget = new WiFiStatusWidget(UIWidgetIds::WiFiStatus,
         _services.displayManager.GetTextureManager(),posWifi);

    _services.displayManager.Send(std::make_unique<AddWidgetCommand>(statusWidget));   

    WidgetPosition posTeam(320-64,0);

    TeamIds team = static_cast<TeamIds>(_services.systemConfig.GetTeamId());

    TextureWidget* textureWidget = nullptr;
    if(!FtpMode())
    {
        textureWidget  = new TextureWidget(UIWidgetIds::TeamID,
            _services.displayManager.GetTextureManager(),posTeam,GetTeamFile(team));
    }
    else            
    {
        textureWidget  = new TextureWidget(UIWidgetIds::TeamID,
            _services.displayManager.GetTextureManager(),posTeam,"/ftp.bmp");  
    }

   _services.displayManager.Send(std::make_unique<AddWidgetCommand>(textureWidget));      
   
   
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

    _network.IndicateFtpMode(FtpMode());

    _connectionShown =
        false;
}

// -----------------------------------------------------------------------------
// Exit
// -----------------------------------------------------------------------------


void AwaitingStartState::Exit()
{
    if (_ftpCallbackInstance == this)
    {
        _ftpCallbackInstance = nullptr;
    }

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

uint16_t AwaitingStartState::Run()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return FtpMode() ? SystemStates::Ftp : SystemStates::AwaitingStart;
    }



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

       
    if(FtpMode())
    {
        if(!_ftpInitDone)
        {
            StartFTP();
        }
        else
        {
            RunFTP();
        }
        _network.Update();  // Keep the network running as this does IP boradcasts
        return SystemStates::Ftp;
    }
    
    
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



void AwaitingStartState::RunFTP()
{
    if(_ftpServer != nullptr)
    {
        _ftpServer->handleFTP();
    }
}


void AwaitingStartState::FtpCallback(
    FtpOperation operation,
    uint32_t freeSpace,
    uint32_t totalSpace)
{
    if (_ftpCallbackInstance != nullptr)
    {
        _ftpCallbackInstance->HandleFtpCallback(
            operation,
            freeSpace,
            totalSpace);
    }
}


void AwaitingStartState::HandleFtpCallback(
    FtpOperation operation,
    uint32_t freeSpace,
    uint32_t totalSpace)
{
    switch (operation)
    {
        case FTP_CONNECT:
            Serial.println("FTP client connected");
            _services.displayManager.EnableRendering(false);
            break;

        case FTP_DISCONNECT:
            Serial.println("FTP client disconnected");
            _services.displayManager.EnableRendering(true);
            break;

        case FTP_FREE_SPACE_CHANGE:
            Serial.print("FTP free space: ");
            Serial.print(freeSpace);
            Serial.print(" / ");
            Serial.println(totalSpace);
            break;
    }
}



void AwaitingStartState::StartFTP()
{
    if(_ftpServer == nullptr)
    {
        Serial.println("Starting FTP server");
        _ftpServer = new FtpServer();

        _ftpCallbackInstance = this;

        _ftpServer->setCallback(
            AwaitingStartState::FtpCallback);

        _ftpServer->begin("combatlr", "password");
        _ftpInitDone = true;
    }
}


bool AwaitingStartState::FtpMode()
{
    return false;
}
