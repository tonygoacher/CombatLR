#include <Arduino.h>


#include "SteppedZoom.h"
#include "countof.h"
#include "MAX98357ASounds.h"
#include <SPIFFS.h>

#include "Ports.h"
MAX98357ASounds sounds(SPIFFS,BCLK,LRC,DATA);

SteppedZoom::SteppedZoom(int port)
: _switch(port),
_current(0),
_last(-1)
{
    Serial.println("AAAAAAAAAAAAAAAA");
    sounds.Begin();
  
}

bool SteppedZoom::GetZoom(float& newZoom)
{
    bool result = false;
    static const float zooms[]= {0.8f, 1.5f, 2.0f, 2.8f, 3.3f, 4.0f};
    if(_switch.Pressed())
    {
        sounds.Play("/sounds/milshot.wav");
        _current++;
        if(_current >= countof(zooms))
        {
            _current = 0;
        }
        Serial.printf("Zoom is ");Serial.println(_current);
        Serial.println(countof(zooms));
    }

    if(_current != _last)
    {
        _last = _current;
        newZoom = zooms[_current];
        result = true;
    }
    return result;
}
