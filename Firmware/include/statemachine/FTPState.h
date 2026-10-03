#include "statemachine/AwaitingStartState.h"

#include "stateMachine/Services.h"


class FtpState : public AwaitingStartState
{
public:
    FtpState(CombatLRServices& services):
        AwaitingStartState(services){};
        
    virtual bool FtpMode() override { Serial.println("FTP MODE FALSE");return true;}  
};