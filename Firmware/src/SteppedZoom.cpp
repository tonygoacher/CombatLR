#include <Arduino.h>


#include "SteppedZoom.h"
#include "countof.h"

#include <LittleFS.h>



SteppedZoom::SteppedZoom(PBSwitch& zoomSwitch)
:  _switch(zoomSwitch),
_current(0),
_last(-1)
{

}



bool SteppedZoom::GetZoom(float& newZoom)
{
    bool result = false;
    static const float zooms[]= {0.8f, 1.5f, 2.0f, 2.8f, 3.3f, 4.0f};
    if(_switch.Pressed())
    {
    
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

void SteppedZoom::Reset()
{
    _current=0;
}

