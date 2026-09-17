#pragma once


#include "State.h"
#include "stdint.h"


#include "stateMachine/Services.h"



class DeadState : public State
{
public:
       
    DeadState( CombatLRServices& stateData);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:    
    CombatLRServices& _stateData;

  
};
