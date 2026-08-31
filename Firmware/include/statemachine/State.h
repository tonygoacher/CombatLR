#pragma once

#include <stdint.h>

class ZoneEngine;
class DisplayManager;
class ISounds;


class State
{
public:
    State(){};


    virtual void Entry() = 0;
    virtual uint16_t Run() = 0;
    virtual void Exit() = 0;

};