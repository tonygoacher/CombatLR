#pragma once

#include "IZoom.h"




class ADCZoom : public IZoom
{
public:
    ADCZoom(int port);
    virtual float GetZoom() override; 

private:
    int _port;
    float _target;
    float _lastValue;
        

};