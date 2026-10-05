#include <ArduinoOTA.h>

#include "statemachine/AwaitingStartState.h"

#include "stateMachine/Services.h"


class OTAState : public AwaitingStartState
{
public:
    OTAState(CombatLRServices& services);

        
    void Entry() override;
    void Exit() override;
    uint16_t Run() override;

private:
    static OTAState* _otaCallbackInstance;

    void OTAStartCallback();
    void OTAErrorCallback(ota_error_t error);
    void OTAProgress(unsigned int progress, unsigned int total);

    void RunOTA();
    void StartOTA();

    bool _otaInitDone;

};

