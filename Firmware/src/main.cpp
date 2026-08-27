#include <Arduino.h>
#include <memory>
#include "ESP32LaserTag.h"
#include "Ports.h"
#include "SPIFFS.h"
#include "FS.h"
#include "NavigationMapTest.h"
#include "NavigationDisplayTest.h"

#include <SPIFFS.h>

#include "BMPImage.h"
#include "IDisplay.h"
#include "ST7789VDisplay.h"
#include "TestWidget.h"
#include "DisplayManager.h"
#include "ZoomCommand.h"
#include "DisplayBackgroundCommand.h"


BMPImage mapdata;

TextureManager textureManager(SPIFFS);
DisplayManager displayManager(textureManager);

void fatal()
{
    while(1)
    {};
}


void setup()
{
    Serial.begin(115200);


    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS failed");
        fatal();
    }

    Serial.printf(
    "Free PSRAM: %u bytes\n",
    ESP.getFreePsram());


    if (!mapdata.Load(
            SPIFFS,
            "/map.bmp"))
    {
        Serial.println("Map load failed");
        
        fatal();
    }



    displayManager.Begin();

    displayManager.Send(
    std::unique_ptr<IDisplayCommand>(
        new DisplayBackgroundCommand(
            "/map.bmp")));

    NavigationDisplayTestBegin(displayManager);

}

int rot = 0;
float zoom =1.0f;
float zoomDir = 0.0f;

void loop()
{

    UpdateNavigationDisplay(displayManager);

  
}