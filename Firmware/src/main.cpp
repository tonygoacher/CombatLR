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

int rot = 0;
float zoom =1.0f;
float zoomDir = 0.0f;

void loop()
{

    UpdateNavigationDisplay();

  
}