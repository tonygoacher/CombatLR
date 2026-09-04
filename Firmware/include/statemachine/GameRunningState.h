#pragma once


#include "State.h"
#include "stdint.h"
#include "Pacer.h"
#include "stateMachine/Services.h"
#include "Navigation.h"
#include "GameCoordinate.h"
#include "widgets/TextWidget.h"
#include "stateMachine/Services.h"
#include "weapons/WeaponManager.h"

class GameRunningState : public State
{
public:
       
    GameRunningState(uint16_t gameTime /*secs*/, CombatLRServices& stateData, WeaponManager& weaponManager);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:    
    uint16_t    _gameTime;
    CombatLRServices& _stateData;
    WeaponManager& _weaponManager;
    GameCoordinate _playerPosition;

};