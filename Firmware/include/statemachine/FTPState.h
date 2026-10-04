#include "statemachine/AwaitingStartState.h"

#include "stateMachine/Services.h"


class FtpState : public AwaitingStartState
{
public:
    FtpState(CombatLRServices& services);

        
    void Entry() override;
    void Exit() override;
    uint16_t Run() override;

private:
    static FtpState* _ftpCallbackInstance;
    static void FtpCallback(
        FtpOperation operation,
        uint32_t freeSpace,
        uint32_t totalSpace);

    void HandleFtpCallback(
        FtpOperation operation,
        uint32_t freeSpace,
        uint32_t totalSpace);    

    void RunFTP();
    void StartFTP();

    FtpServer *  _ftpServer;
    bool _ftpInitDone;

};