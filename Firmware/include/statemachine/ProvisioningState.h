#pragma once

#include "statemachine/AwaitingStartState.h"
#include <WebServer.h>

class ProvisioningState : public AwaitingStartState
{
public:
    ProvisioningState(CombatLRServices& services);

    void Entry() override;
    void Exit() override;
    uint16_t Run() override;

private:
    void StartAccessPoint();
    void StartWebServer();
    void StopWebServer();

    void HandleRoot();
    void HandleSave();
    void HandleNotFound();

    String BuildPage();
    String HtmlEscape(const String& value);
    String GetArg(const char* name);

    WebServer* _webServer;
    bool _webServerStarted;
    bool _saveRequested;

    IPAddress _apIP;
};
