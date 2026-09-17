
#include "ISounds.h"
#include "statemachine/SystemStates.h"
#include "stateMachine/DeadState.h"
#include "graphics/DisplayManager.h"
#include "graphics/DisplayBackgroundCommand.h"




DeadState::DeadState( CombatLRServices& stateData)
:_stateData(stateData)
{
    
}



void DeadState::Entry()
{
    _stateData.sounds.PlayPriority("/sounds/dead.wav");
    _stateData.displayManager.Send(
    std::unique_ptr<IDisplayCommand>(
        new DisplayBackgroundCommand(
            "/dead.bmp")));
}





void DeadState::Exit()
{

}



uint16_t DeadState::Run()
{
    return SystemStates::Dead;
}

