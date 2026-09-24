#pragma once


#include "State.h"
#include "stdint.h"
#include "Pacer.h"


class DisplayManager;
class INavigation ;


class GPSLockState : public State
{
public:
       
    GPSLockState(DisplayManager& displayManager, INavigation& navigation);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:    

    static const int TextX = 160;
    static const int TextY=180;
    static const int TextZoom = 2;
    static const int TextFont = 2;  

    INavigation& _navigation;
    DisplayManager& _display;
    Pacer _pacer;
};