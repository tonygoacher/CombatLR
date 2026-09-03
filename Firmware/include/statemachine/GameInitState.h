#pragma once


#include "State.h"
#include "stdint.h"
#include "Pacer.h"

#include "StateData.h"
#include "Navigation.h"
#include "GameCoordinate.h"
#include "widgets/TextWidget.h"


class GameInitState : public State
{
public:
       
    GameInitState(uint32_t timeToGameStart /*secs*/, StateData& stateData);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:    
    StateData& _stateData;

    Pacer _textPacer;

    static const int TextX = 280;
    static const int TextY=10;
    static const int TextZoom = 2;
    static const int TextFont = 2;      // Must include appropriate -D LOAD_FONT2  if this changes

    uint16_t _timeToGameStart;
    GameCoordinate _playerPosition;
    TextWidget* _textWidget;

    float  _lastZoom;
};