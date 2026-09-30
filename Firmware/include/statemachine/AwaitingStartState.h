#pragma once


#include "State.h"
#include "stdint.h"
#include "CombatLRNetwork.h"
#include "TeamIDs.h"


class DisplayManager;
class INavigation ;
class CombatLRServices;


class AwaitingStartState : public State
{
public:
       
    AwaitingStartState(CombatLRServices& services);
    uint16_t Run() override;
    void Entry() override;
    void Exit() override;

private:    
    const char * const GetTeamFile(TeamIds teamId);
    CombatLRServices& _services;
    CombatLRNetwork _network;
    bool _connectionShown;


    static const int TextX = 160;
    static const int TextY=180;
    static const int TextZoom = 2;
    static const int TextFont = 2;  
};