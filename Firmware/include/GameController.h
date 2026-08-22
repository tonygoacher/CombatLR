#pragma once

#include "GamePlan.h"
#include "IGameTime.h"

class GameController
{
public:
    static GameController& Instance();

    GameController(
        const GameController&) = delete;

    GameController& operator=(
        const GameController&) = delete;

    void Initialise(
        const GamePlan& gamePlan,
        IGameTime& gameTime);

private:
    GameController();

    const GamePlan* _gamePlan;
    IGameTime* _gameTime;
};