#pragma once


#include "State.h"
#include "stdint.h"
#include "Pacer.h"
#include "Services.h"   



class DisplayManager;


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

    static const int TextX = 160;
    static const int TextY=180;
    static const int TextZoom = 2;
    static const int TextFont = 2;  

};