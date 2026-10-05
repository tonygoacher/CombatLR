#include <WiFi.h>
#include <WiFiUdp.h>

#include "statemachine/OTAState.h"
#include "graphics/DisplayManager.h"
#include "graphics/TextureManager.h"
#include "widgets/TextureWidget.h"
#include "widgets/UIWidgetIds.h"
#include "statemachine/SystemStates.h"
#include "graphics/AddWidgetCommand.h"
#include "PacedLED.h"


OTAState* OTAState::_otaCallbackInstance = nullptr;

/*
    Using PlatformIO. Click the terminal icon in the bottom left status bar of VSCode
    Boot the CombatLR unit with trigger and zom buttons held down (hold for ~3 secs)
    In the termional,run
     pio run -e esp32s3_n16r8 -t upload --upload-port ipaddress
     e.g.
     pio run -e esp32s3_n16r8 -t upload --upload-port 192.168.7.145
*/


OTAState::OTAState(CombatLRServices& services):
    AwaitingStartState(services),
    _otaInitDone(false)
{
    
}


void OTAState::Entry()
{
    EntryCommon();
    _network.IndicateFtpMode(false);

    _otaCallbackInstance = this;

    TextureWidget* textureWidget = nullptr;
     WidgetPosition posFTP(320-64,0);

    textureWidget  = new TextureWidget(UIWidgetIds::TeamID,
        _services.displayManager.GetTextureManager(),posFTP,"/ota.bmp");  
    _services.displayManager.Send(std::make_unique<AddWidgetCommand>(textureWidget));    

    ArduinoOTA.onStart([](){ if(_otaCallbackInstance != nullptr){_otaCallbackInstance->OTAStartCallback();}});
    ArduinoOTA.onError([](ota_error_t error) {
        if (_otaCallbackInstance != nullptr) {
            _otaCallbackInstance->OTAErrorCallback(error);
        }});

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (_otaCallbackInstance != nullptr) {
                _otaCallbackInstance->OTAProgress(progress, total);
        }});
}


void OTAState::Exit() 
{
    if (_otaCallbackInstance == this)
    {
        _otaCallbackInstance = nullptr;
    }
}

uint16_t OTAState::Run() 
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return SystemStates::OTA;
    }

    RunCommon();

    if(!_otaInitDone)
    {
        StartOTA();
    }
    else
    {
        RunOTA();
    }
    return SystemStates::OTA;
} 




void OTAState::RunOTA()
{
    ArduinoOTA.handle();
}




void OTAState::StartOTA()
{
    ArduinoOTA.begin();
    _otaInitDone = true;
}


void OTAState::OTAProgress(unsigned int progress, unsigned int total)
{
    unsigned int percent = (progress / (total / 100));
    _services.hitLED.Run();
    if((percent  % 5) == 0)
    {
        _services.hitLED.Fire(20);
    }
}



void OTAState::OTAStartCallback()
{
    Serial.println("OTA Starting");
    _services.displayManager.EnableRendering(false);
}


void OTAState::OTAErrorCallback(ota_error_t error)
{
    Serial.printf("OTA Error %d\n", error);
    _services.displayManager.EnableRendering(true);
}


