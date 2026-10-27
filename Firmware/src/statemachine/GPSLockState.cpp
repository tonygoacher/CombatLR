#include <stdio.h>
#include "statemachine/GPSLockState.h"
#include "statemachine/CombatLRStateMachine.h"
#include "statemachine/GenericEvent.h"
#include "ISounds.h"
#include "graphics/DisplayManager.h"
#include "Navigation.h"
#include "graphics/DisplayBackgroundCommand.h"
#include  "statemachine/SystemStates.h"
#include "graphics/AddWidgetCommand.h"
#include "graphics/SetTextCommand.h"
#include "graphics/RemoveWidgetCommand.h"

#include "widgets/GPSStatusWidget.h"
#include "widgets/TextWidget.h"
#include "widgets/UIWidgetIds.h"


GPSLockState::GPSLockState(CombatLRServices& services)
: _services(services)
{  
    
}


void GPSLockState::SetUpdateTimer()
{
    GenericEvent textUpdate(GPSLockStateEvents::UpdateHDOP);
    _services.stateMachine.ScheduleEvent(textUpdate,1000);
}


uint16_t GPSLockState::Run()
{
    _services.navigation.Update();
    auto event = _services.stateMachine.GetEvent();
    if(event && event->GetType() == GPSLockStateEvents::UpdateHDOP)
    {
        SetUpdateTimer();
        char gpsInfo[128];
        snprintf(gpsInfo, 128, "HDOP:%.3f", _services.navigation.GetHDOP());
        SetTextCommand* setText = new SetTextCommand(UIWidgetIds::GPSInfo, gpsInfo);
        _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(setText));
    }
    if(_services.navigation.HasPosition())
    {
        return SystemStates::AwaitingStart;
    }
    return SystemStates::GPSLock;
}

void GPSLockState::Entry()
{

    _services.displayManager.Send(
    std::unique_ptr<IDisplayCommand>(
        new DisplayBackgroundCommand(
            "/logo.bmp")));

    WidgetPosition p(0,0);

    GPSStatusWidget* statusWidget = new GPSStatusWidget(UIWidgetIds::GPStatus,  
         _services.displayManager.GetTextureManager(),p, _services.navigation);
    AddWidgetCommand* command =
    new AddWidgetCommand(statusWidget);     // NOTE GPS widget stays onscreen so not removed in ::Exit()

   _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(command));

    WidgetPosition gpsText(TextX,TextY);
    TextWidget* textWidget = new TextWidget(UIWidgetIds::GPSInfo,"",gpsText,TextFont, MC_DATUM, TFT_WHITE, TFT_BLACK, TextZoom );
    command = new AddWidgetCommand(textWidget);
    _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(command));

    SetUpdateTimer();

}
void GPSLockState::Exit()
{
     _services.stateMachine.CancelAllEvents();
    RemoveWidgetCommand* remove = new RemoveWidgetCommand(UIWidgetIds::GPSInfo);
     _services.displayManager.Send(std::unique_ptr<IDisplayCommand>(remove));   

}
