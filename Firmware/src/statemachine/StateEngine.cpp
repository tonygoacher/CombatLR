#include <Arduino.h>
#include "statemachine/StateEngine.h"




StateEngine::StateEngine()
: _currentState(0)
{
    _currentState = _states.end();
}




void StateEngine::Run()
{
    if(_currentState != _states.end())
    {
        uint16_t nextState = _currentState->second.Run();
        if(nextState != _currentState->first)
        {

            StateMap::iterator iterNextState = _states.find(nextState);
            if(iterNextState == _states.end())
            {
                Serial.printf("FATAL: StateEngine no state %d found\n", nextState);
                while(1){};
            }

            Serial.printf("Exiting state %d\n", _currentState->first);
            _currentState->second.Exit();
 

            _currentState = iterNextState;
            Serial.printf("Entering state %d\n", nextState);
            _currentState->second.Entry();
        }
    }
    else
    {
        Serial.println("StateEngine::Run Current is invalid");
    }
}
