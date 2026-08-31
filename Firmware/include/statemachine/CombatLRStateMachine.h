#pragma once



#include "StateEngine.h"
#include "StateData.h"





class CombatLRStateMachine : public StateEngine
{
public:
    CombatLRStateMachine(StateData& stateData);
    void Init() ;
    void Start() override;

private:
    StateData& _stateData;
};


