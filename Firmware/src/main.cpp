#include <Arduino.h>
#include <memory>
#include "ESP32LaserTag.h"
#include "Ports.h"
#include <LittleFS.h>
#include "FS.h"
#include "NavigationMapTest.h"
#include "NavigationDisplayTest.h"

#include <LittleFS.h>

#include "BMPImage.h"
#include "IDisplay.h"
#include "ST7789VDisplay.h"
#include "TestWidget.h"
#include "DisplayManager.h"
#include "ZoomCommand.h"
#include "DisplayBackgroundCommand.h"
#include "LittleFS.h"


BMPImage mapdata;

TextureManager textureManager(LittleFS);
DisplayManager displayManager(textureManager);

void fatal()
{
    while(1)
    {};
}


#include "esp_heap_caps.h"

void PrintPSRAM(const char* label)
{
    Serial.printf(
        "PSRAM %-20s free=%u largest=%u\n",
        label,
        ESP.getFreePsram(),
        heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

void setup()
{
    Serial.begin(115200);

    PrintPSRAM("Before LITTLEFS");
  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  PrintPSRAM("After LITTLEFS");

        
    if (!mapdata.Load(
            LittleFS,
            "/map.bmp"))
    {
        Serial.println("Map load failed");
        
        fatal();
    }

    PrintPSRAM("After mapdata.load");



    displayManager.Begin();
     PrintPSRAM("After displaymanager.begin");

    displayManager.Send(
    std::unique_ptr<IDisplayCommand>(
        new DisplayBackgroundCommand(
            "/map.bmp")));

    PrintPSRAM("Before NavigationDisplayTestBegin");
    NavigationDisplayTestBegin(displayManager);
    PrintPSRAM("After NavigationDisplayTestBegin");

}

int rot = 0;
float zoom =1.0f;
float zoomDir = 0.0f;

void loop()
{

    UpdateNavigationDisplay(displayManager);

  
}