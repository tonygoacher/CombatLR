#include "statemachine/FTPState.h"
#include "graphics/DisplayManager.h"
#include "graphics/TextureManager.h"
#include "widgets/TextureWidget.h"
#include "widgets/UIWidgetIds.h"
#include "statemachine/SystemStates.h"
#include "graphics/AddWidgetCommand.h"


FtpState* FtpState::_ftpCallbackInstance = nullptr;


FtpState::FtpState(CombatLRServices& services):
    AwaitingStartState(services),
    _ftpServer(nullptr), _ftpInitDone(false)
{
    
}


void FtpState::Entry()
{
    EntryCommon();
    _network.IndicateFtpMode(true);

    _ftpCallbackInstance = this;

    TextureWidget* textureWidget = nullptr;
     WidgetPosition posFTP(320-64,0);

    textureWidget  = new TextureWidget(UIWidgetIds::TeamID,
        _services.displayManager.GetTextureManager(),posFTP,"/ftp.bmp");  
    _services.displayManager.Send(std::make_unique<AddWidgetCommand>(textureWidget));    
}


void FtpState::Exit() 
{
    if (_ftpCallbackInstance == this)
    {
        _ftpCallbackInstance = nullptr;
    }
}

uint16_t FtpState::Run() 
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return SystemStates::Ftp;
    }

    RunCommon();

    if(!_ftpInitDone)
    {
        StartFTP();
    }
    else
    {
        RunFTP();
    }
    _network.Update();  // Keep the network running as this does IP broadcasts
    return SystemStates::Ftp;
} 




void FtpState::RunFTP()
{
    if(_ftpServer != nullptr)
    {
        _ftpServer->handleFTP();
    }
}


void FtpState::FtpCallback(
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


void FtpState::HandleFtpCallback(
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



void FtpState::StartFTP()
{
    if(_ftpServer == nullptr)
    {
        Serial.println("Starting FTP server");
        _ftpServer = new FtpServer();

        _ftpCallbackInstance = this;

        _ftpServer->setCallback(
            FtpState::FtpCallback);

        _ftpServer->begin("combatlr", "password");
        _ftpInitDone = true;
    }
}
