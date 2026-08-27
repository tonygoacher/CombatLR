#include <Arduino.h>
#include "ADCZoom.h"

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
 return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

ADCZoom::ADCZoom(int port)
: _port(port)
{
    pinMode(_port,ANALOG);  
    _lastValue = 1.0f;
}

float ADCZoom::GetZoom()
{
    int adcValue = analogRead(_port);
    _target =  mapfloat((float)adcValue, (float)0 , (float)4095, IZoom::_minZoom, IZoom::_maxZoom);  
    float delta = _target - _lastValue; 
    _lastValue += (delta * 0.1);
    int convert =  (_lastValue * 10.0f);
    return float((float)convert / 10.0f);
}
