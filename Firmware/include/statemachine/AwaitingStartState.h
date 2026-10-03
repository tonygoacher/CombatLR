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
    virtual bool  FtpMode();

private:  
  
    void RunFTP();
    void StartFTP();
    const char * const GetTeamFile(TeamIds teamId);
    CombatLRServices& _services;
    CombatLRNetwork _network;
    FtpServer *  _ftpServer;
    bool _connectionShown;
    bool _networkShudown;

    bool _ftpInitDone;
    Pacer _exitPacer;


    static const int TextX = 160;
    static const int TextY=180;
    static const int TextZoom = 2;
    static const int TextFont = 2;  
};

