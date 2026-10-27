
#include "statemachine/PowerUpState.h"
#include "ISounds.h"
#include "graphics/DisplayManager.h"
#include "Navigation.h"
#include "graphics/DisplayBackgroundCommand.h"
#include  "statemachine/SystemStates.h"
#include "graphics/AddWidgetCommand.h"
#include "widgets/GPSStatusWidget.h"
#include "widgets/UIWidgetIds.h"
#include "PBSwitch.h"
#include "statemachine/CombatLRStateMachine.h"
#include "statemachine/GenericEvent.h"




PowerUpState::PowerUpState(CombatLRServices& services)
: _services(services)
{  

}

uint16_t PowerUpState::Run()
{
    _services.navigation.Update();
    auto event = _services.stateMachine.GetEvent();
    if(event && event->GetType() == PowerUpState::MoveToNextState)
    {


        if(_services.triggerSwitch.RawRead() && _services.reloadSwitch.RawRead())
        {
            return SystemStates::Ftp;
        }

        if(_services.triggerSwitch.RawRead() && _services.zoomSwitch.RawRead())
        {
            return SystemStates::OTA;
        }

        if(_services.leftSwitch.RawRead() && _services.rightSwitch.RawRead())
        {
            return SystemStates::Provisioning;
        }
            
        return SystemStates::GPSLock;

    }
    return SystemStates::PowerUp;
}

void PowerUpState::Entry()
{

    GenericEvent moveToNextStateEvent(PowerUpStateEvents::MoveToNextState);
    _services.stateMachine.ScheduleEvent(moveToNextStateEvent, 2000);


    _services.displayManager.Send(
    std::unique_ptr<IDisplayCommand>(
        new DisplayBackgroundCommand(
            "/logo.bmp")));

    WidgetPosition p(0,0);

    GPSStatusWidget* statusWidget = new GPSStatusWidget(UIWidgetIds::GPStatus,
         _services.displayManager.GetTextureManager(),p, _services.navigation);
    AddWidgetCommand* command =
    new AddWidgetCommand(statusWidget);

    _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(command));

}
void PowerUpState::Exit()
{
 
}
