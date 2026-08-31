
#include "statemachine/CombatLRStateMachine.h"
#include "statemachine/SystemStates.h"

#include "statemachine/PowerUpState.h"
#include "statemachine/GameInitState.h"
#include <Arduino.h>



CombatLRStateMachine::CombatLRStateMachine(StateData& stateData)
:   _stateData(stateData)
{
    
}



void CombatLRStateMachine::Start()
{
    _currentState = _states.find(SystemStates::PowerUp);
    if(_currentState != _states.end())
    {
        _currentState->second.Entry();
    }
    else
    {
        Serial.printf("CombatLRStateMachine::Start unable to find start state");
    }

}



void CombatLRStateMachine::Init()
{
    _states.insert(StateMapEntry(SystemStates::PowerUp, *new PowerUpState(_stateData.displayManager,_stateData.navigation)));
    _states.insert(StateMapEntry(SystemStates::GameInit,*new GameInitState(5000, _stateData)));
}