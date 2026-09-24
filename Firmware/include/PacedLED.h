#pragma once

#include "Pacer.h"





class PacedLED
{
public:
    PacedLED(int port);
    void Fire(int period);
    void Run();

private:
    int     _port;
    Pacer   _pacer;    
};