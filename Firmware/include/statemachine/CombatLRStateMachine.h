#pragma once



#include "StateEngine.h"
#include "statemachine/Services.h"



class WeaponManager;

class CombatLRStateMachine : public StateEngine
{
public:
    CombatLRStateMachine(CombatLRServices& stateData, WeaponManager& weaponManager);
    void Init() ;
    void Start() override;

private:
    CombatLRServices& _stateData;
    WeaponManager& _weaponManager;
  
};


