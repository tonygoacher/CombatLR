
#include "statemachine/CombatLRStateMachine.h"
#include "statemachine/SystemStates.h"

#include "statemachine/AwaitingStartState.h"
#include "statemachine/PowerUpState.h"
#include "statemachine/GameInitState.h"
#include "statemachine/GameRunningState.h"
#include "statemachine/GPSLockState.h"
#include "statemachine/DeadState.h"
#include "weapons/WeaponManager.h"
#include <Arduino.h>



CombatLRStateMachine::CombatLRStateMachine(CombatLRServices& stateData, WeaponManager& weaponManager)
:   _stateData(stateData), _weaponManager(weaponManager)
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
    _states.insert(StateMapEntry(SystemStates::GPSLock, *new GPSLockState(_stateData.displayManager,_stateData.navigation)));
    _states.insert(StateMapEntry(SystemStates::GameInit, *new GameInitState(_stateData)));
    _states.insert(StateMapEntry(SystemStates::Dead,*new DeadState(_stateData)));
    _states.insert(StateMapEntry(SystemStates::AwaitingStart,*new AwaitingStartState(_stateData)));
    _states.insert(StateMapEntry(SystemStates::GameRunning,*new GameRunningState(12000, _stateData, _weaponManager)));
    _states.insert(StateMapEntry(SystemStates::Dead,*new DeadState(_stateData)));
}