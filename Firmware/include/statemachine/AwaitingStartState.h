#pragma once


#include "State.h"
#include "stdint.h"


class DisplayManager;
class INavigation ;


class PowerUpState : public State
{
public:
       
    PowerUpState(DisplayManager& displayManager, INavigation& sounds);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:    
    INavigation& _navigation;
    DisplayManager& _display;
};