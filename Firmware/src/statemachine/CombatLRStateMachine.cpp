
#include "statemachine/CombatLRStateMachine.h"
#include "statemachine/SystemStates.h"

#include "statemachine/AwaitingStartState.h"
#include "statemachine/PowerUpState.h"
#include "statemachine/GameInitState.h"
#include "statemachine/GameRunningState.h"
#include "statemachine/GPSLockState.h"
#include "statemachine/FTPState.h"
#include "statemachine/OTAState.h"
#include "statemachine/ProvisioningState.h"
#include "statemachine/DeadState.h"
#include "statemachine/LTTestState.h"
#include "weapons/WeaponManager.h"
#include <Arduino.h>



CombatLRStateMachine::CombatLRStateMachine()
: StateEngine("GameStateMachine")
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



void CombatLRStateMachine::Init(CombatLRServices& services, WeaponManager& weaponManager)
{

    _states.insert(StateMapEntry(SystemStates::PowerUp, *new PowerUpState(services)));
    _states.insert(StateMapEntry(SystemStates::GPSLock, *new GPSLockState(services)));
    _states.insert(StateMapEntry(SystemStates::GameInit, *new GameInitState(services)));
    _states.insert(StateMapEntry(SystemStates::Dead,*new DeadState(services)));
    _states.insert(StateMapEntry(SystemStates::AwaitingStart,*new AwaitingStartState(services)));
    _states.insert(StateMapEntry(SystemStates::Ftp,*new FtpState(services)));
    _states.insert(StateMapEntry(SystemStates::GameRunning,*new GameRunningState(12000, services, weaponManager)));
    _states.insert(StateMapEntry(SystemStates::Dead,*new DeadState(services)));
    _states.insert(StateMapEntry(SystemStates::OTA,*new OTAState(services)));
    _states.insert(StateMapEntry(SystemStates::Provisioning,*new ProvisioningState(services)));    
    _states.insert(StateMapEntry(SystemStates::IRTestState,*new LTTestState(services)));    
}

