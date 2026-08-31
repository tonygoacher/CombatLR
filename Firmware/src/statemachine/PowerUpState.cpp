
#include "statemachine/PowerUpState.h"
#include "ISounds.h"
#include "DisplayManager.h"
#include "Navigation.h"
#include "DisplayBackgroundCommand.h"
#include  "statemachine/SystemStates.h"
#include "AddWidgetCommand.h"
#include "widgets/GPSStatusWidget.h"
#include "widgets/UIWidgetIds.h"


PowerUpState::PowerUpState(DisplayManager& displayManager, INavigation& navigation)
: _display(displayManager),
 _navigation(navigation), _pacer(true)
{  

}

uint16_t PowerUpState::Run()
{
    _navigation.Update();
    if(_pacer.Pace())
    {
        Serial.println("Exiting PowerUpState::Run()");
        return SystemStates::GameInit;
    }
    return SystemStates::PowerUp;
}

void PowerUpState::Entry()
{
    _pacer.SetPace(5000);


    _display.Send(
    std::unique_ptr<IDisplayCommand>(
        new DisplayBackgroundCommand(
            "/logo.bmp")));

    WidgetPosition p(0,0);

    GPSStatusWidget* statusWidget = new GPSStatusWidget(UIWidgetIds::GPStatus,
         _display.GetTextureManager(),p, _navigation);
    AddWidgetCommand* command =
    new AddWidgetCommand(statusWidget);

    _display.Send(std::unique_ptr<IDisplayCommand>(command));

}
void PowerUpState::Exit()
{
    
}
