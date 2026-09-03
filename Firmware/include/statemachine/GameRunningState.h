#pragma once


#include "State.h"
#include "stdint.h"
#include "Pacer.h"

#include "StateData.h"
#include "Navigation.h"
#include "GameCoordinate.h"
#include "widgets/TextWidget.h"
#include "statemachine/StateData.h"
#include "weapons/AssaultRifle.h"

class GameRunningState : public State
{
public:
       
    GameRunningState(uint16_t gameTime /*secs*/, StateData& stateData);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:    
    uint16_t    _gameTime;
    StateData& _stateData;
    GameCoordinate _playerPosition;
    AssaultRifle _ar;
    
};