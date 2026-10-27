#pragma once



#include "StateEngine.h"
#include "statemachine/Services.h"



class WeaponManager;

class CombatLRStateMachine : public StateEngine
{
public:
    CombatLRStateMachine();
    void Init(CombatLRServices& services, WeaponManager& weaponManager);
    void Start() override;

};


