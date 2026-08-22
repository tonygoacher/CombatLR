#include <Arduino.h>
#include "ESP32LaserTag.h"
#include "Ports.h"
#include "SPIFFS.h"
#include "FS.h"
#include "JSONConfig.h"
#include "NavigationMapTest.h"
#include "NavigationDisplayTest.h"
/*
ESP32LaserTag laser(TXPORT, RXPORT);
JSONConfig* jsonConfig = nullptr;

void setup()
{
    Serial.begin(115200);
    delay(2000);
    laser.Init();
    Serial.println("Started.....");

    if (!SPIFFS.begin(true))
    {
        Serial.println("Failed to mount SPIFFS");
        while(1)
        {};
    }

    jsonConfig = new JSONConfig(SPIFFS);
    if(jsonConfig->Load())
    {
        Serial.print("Max Game Time:"); Serial.println(jsonConfig->GetConfig()->maxGameTime);
    }        

}


void loop()
{
    LaserTagData d;
    d.damage=1;
    d.playerId=1;
    d.team = 1;


    laser.SendLasertagData(d);
    d = {0,0,0};
    if(laser.LaserTagDataAvailable())
    {
        if(laser.GetLasertagData(d))
        {
            Serial.println(" ");
            Serial.print("Team   :"); Serial.println(d.team);
            Serial.print("Damage :"); Serial.println(d.damage);
            Serial.print("Player :"); Serial.println(d.playerId);
            Serial.println(" ");
        }
    }
    
    delay(1000);
    Serial.print(".");

}

*/
#include <SPIFFS.h>

#include "BMPImage.h"
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
/*
display->AddWidget(
    new TestWidget(
        WidgetCoordinateSpace::Map,
        50,
        50,
        TFT_RED));

display->AddWidget(
    new TestWidget(
        WidgetCoordinateSpace::Map,
        0,
        0,
        TFT_GREEN));
*/
//    display->SetMapZoom(2.0);
  //  display->Update();

   NavigationDisplayTestBegin();
}

int rot = 0;
float zoom =1.0f;
float zoomDir = 0.0f;

void loop()
{

    NavigationDisplayTestLoop();
    display->SetRotation(rot);

    display->SetMapZoom(zoom);

    display->Update();   
    
  
}