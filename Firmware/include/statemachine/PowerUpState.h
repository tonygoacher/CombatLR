#pragma once


#include "State.h"
#include "stdint.h"
#include "Pacer.h"
#include "Services.h"   



class DisplayManager;
class INavigation ;


class PowerUpState : public State
{
public:
       
    PowerUpState(CombatLRServices& services);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:    
    enum PowerUpStateEvents
    {
        MoveToNextState,
    };
    CombatLRServices& _services;

};