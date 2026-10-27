#pragma once


#include "State.h"
#include "stdint.h"
#include "Pacer.h"
#include "statemachine/Services.h"



class GPSLockState : public State
{
public:
       
    GPSLockState(CombatLRServices& services);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:  
    
    enum GPSLockStateEvents
    {
        UpdateHDOP,
    };

    void SetUpdateTimer();

    static const int TextX = 160;
    static const int TextY=180;
    static const int TextZoom = 2;
    static const int TextFont = 2;  

    CombatLRServices& _services;

};