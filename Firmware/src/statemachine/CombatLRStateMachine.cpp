
#include "statemachine/CombatLRStateMachine.h"
#include "statemachine/SystemStates.h"

#include "statemachine/PowerUpState.h"
#include <Arduino.h>



CombatLRStateMachine::CombatLRStateMachine(DisplayManager& display, ISounds& sounds, ZoneEngine& zoneEngine, INavigation& navigation)
:   _display(display),
    _sounds(sounds),
    _zoneEngine(zoneEngine),
    _navigation(navigation)
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
    _states.insert(StateMapEntry(SystemStates::PowerUp, *new PowerUpState(_display,_navigation)));
}