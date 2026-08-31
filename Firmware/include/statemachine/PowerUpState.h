#pragma once


#include "State.h"
#include "stdint.h"
#include "Pacer.h"


class DisplayManager;
class INavigation ;


class PowerUpState : public State
{
public:
       
    PowerUpState(DisplayManager& displayManager, INavigation& navigation);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:    
    INavigation& _navigation;
    DisplayManager& _display;
    Pacer _pacer;
};