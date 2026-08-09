#include <Arduino.h>
#include "ESP32LaserTag.h"
#include "Ports.h"


ESP32LaserTag laser(TXPORT, RXPORT);

void setup()
{
    Serial.begin(115200);
    laser.Init();
    Serial.println("Started.....");

}


void loop()
{
    LaserTagData d;
    d.damage=1;
    d.playerId=1;
    d.team = 0;


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
    
    delay(200);
    Serial.print(".");

}