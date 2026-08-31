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
            Serial.printf("Exiting state %d\n", _currentState->first);
            _currentState->second.Exit();

            StateMap::iterator iterNextState = _states.find(nextState);
 
            if(iterNextState != _states.end())
            {
                _currentState = iterNextState;
                Serial.printf("Entering state %d", nextState);
                _currentState->second.Entry();
            }
        }
    }
    else
    {
        Serial.println("StateEngine::Run Current is invalid");
    }

}
