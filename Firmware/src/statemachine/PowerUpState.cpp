
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


PowerUpState::PowerUpState(CombatLRServices& services)
: _services(services),
_pacer(true)
{  
    _pacer.SetPace(2000);
}

uint16_t PowerUpState::Run()
{
    _services.navigation.Update();
    if(_pacer.Pace())
    {
        Serial.printf("T:%d R:%d\n", _services.triggerSwitch.RawRead(), _services.reloadSwitch.RawRead());
        if(_services.triggerSwitch.RawRead() && _services.reloadSwitch.RawRead())
        {
            return SystemStates::Ftp;
        }
        else
        {
            return SystemStates::GPSLock;
        }
    }
    return SystemStates::PowerUp;
}

void PowerUpState::Entry()
{
    _pacer.SetPace(2000);


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
