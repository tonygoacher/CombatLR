#include <Arduino.h>
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


BMPImage mapdata;
ST7789VDisplay* display = nullptr;

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


    display = new ST7789VDisplay(
        mapdata.GetData(),
        mapdata.GetWidth(),
        mapdata.GetHeight());


    if (!display->Init())
    {
        Serial.println("Display init failed");
        fatal();
    }



    NavigationDisplayTestBegin(display);
}

int rot = 0;
float zoom =1.0f;
float zoomDir = 0.0f;

void loop()
{

    NavigationDisplayTestLoop();
    display->SetRotation(rot);

    display->Update();   
    
  
}