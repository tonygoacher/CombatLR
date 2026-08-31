#pragma once



#include "StateEngine.h"



class DisplayManager;
class ISounds;
class ZoneEngine;
class INavigation;

class CombatLRStateMachine : public StateEngine
{
public:
    CombatLRStateMachine(DisplayManager& display, ISounds& sounds, ZoneEngine& zoneEngine, INavigation& navigation);
    void Init() ;
    void Start() override;

private:
    DisplayManager& _display;
    ISounds& _sounds; 
    ZoneEngine& _zoneEngine;
    INavigation& _navigation;
};


