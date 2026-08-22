#include "GameController.h"
#include "IGameTime.h"
GameController& GameController::Instance()
{
    static GameController instance;
    return instance;
}

GameController::GameController()
    : _gamePlan(nullptr),
      _gameTime(nullptr)
{
}


void GameController::Initialise(
    const GamePlan& gamePlan,
    IGameTime& gameTime)
{
    _gamePlan = &gamePlan;
    _gameTime = &gameTime;
}