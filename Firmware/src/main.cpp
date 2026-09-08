#include <Arduino.h>
#include <memory>
#include "ESP32LaserTag.h"
#include "Ports.h"
#include <LittleFS.h>
#include "FS.h"

#include "NavigationDisplayTest.h"

#include <LittleFS.h>

#include "graphics/BMPImage.h"
#include "graphics/IDisplay.h"
#include "graphics/ST7789VDisplay.h"
#include "graphics/DisplayManager.h"
#include "graphics/ZoomCommand.h"
#include "graphics/DisplayBackgroundCommand.h"
#include "LittleFS.h"





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

 
    PrintPSRAM("After mapdata.load");


  




    PrintPSRAM("Before NavigationDisplayTestBegin");
    NavigationDisplayTestBegin();
    PrintPSRAM("After NavigationDisplayTestBegin");

}




void loop()
{


    UpdateNavigationDisplay();


                      
}