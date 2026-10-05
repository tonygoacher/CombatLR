#pragma once


#include "State.h"
#include "stdint.h"
#include "CombatLRNetwork.h"
#include "TeamIDs.h"
#include "Pacer.h"
#include <SimpleFTPServer.h>


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
protected:
    void EntryCommon();
    void RunCommon();
    void StartWifiConnection();
    void HandleWifiConnection();

 

    const char * const GetTeamFile(TeamIds teamId);
    CombatLRServices& _services;
    CombatLRNetwork _network;

    bool _connectionShown;
    bool _networkShudown;

    uint8_t _wifiIndex;
    uint32_t _wifiAttemptStart;

    static constexpr uint32_t WifiAttemptTimeoutMs = 20000;


    Pacer _exitPacer;
    String _ssid;


    static const int TextX = 160;
    static const int TextY=180;
    static const int TextZoom = 2;
    static const int TextFont = 2;  
};

