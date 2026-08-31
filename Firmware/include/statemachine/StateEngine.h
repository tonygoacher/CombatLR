#pragma once

#include "State.h"
#include <map>

class StateEngine
{
public:
    StateEngine();
    virtual void Start()=0;
    void Run();


protected:
    typedef std::pair<uint16_t, State&> StateMapEntry;
    typedef std::map<uint16_t, State&> StateMap;

    StateMap _states;
    StateMap::iterator _currentState;

    static const int FirstState = 0;
};



