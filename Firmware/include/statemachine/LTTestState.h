#pragma once


#include "State.h"
#include "stdint.h"

#include "Services.h"   





class LTTestState : public State
{
public:
       
    LTTestState(CombatLRServices& services);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:    

    static const int TextX = 160;
    static const int TextY=180;
    static const int TextZoom = 2;
    static const int TextFont = 2;  

    CombatLRServices& _services;


    static const int TimeToClearLTData = 1000;
    static const uint8_t DefaultWeaponIndex = 0;
    static const int InterShotTime = 100;
    static const size_t LTDataTextSize = 16;

};