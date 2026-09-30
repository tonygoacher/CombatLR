#include <stdio.h>
#include "statemachine/GPSLockState.h"
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


GPSLockState::GPSLockState(DisplayManager& displayManager, INavigation& navigation)
: _display(displayManager),
 _navigation(navigation), _pacer(true)
{  
    
}

uint16_t GPSLockState::Run()
{
    _navigation.Update();
    if(_pacer.Pace())
    {

        char gpsInfo[128];
        snprintf(gpsInfo, 128, "HDOP:%.3f", _navigation.GetHDOP());
        SetTextCommand* setText = new SetTextCommand(UIWidgetIds::GPSInfo, gpsInfo);
        _display.Send(std::unique_ptr<IDisplayCommand>(setText));
    }
    if(_navigation.HasPosition())
    {
        return SystemStates::AwaitingStart;
    }
    return SystemStates::GPSLock;
}

void GPSLockState::Entry()
{
    _pacer.SetPace(1000);


    _display.Send(
    std::unique_ptr<IDisplayCommand>(
        new DisplayBackgroundCommand(
            "/logo.bmp")));

    WidgetPosition p(0,0);

    GPSStatusWidget* statusWidget = new GPSStatusWidget(UIWidgetIds::GPStatus,  
         _display.GetTextureManager(),p, _navigation);
    AddWidgetCommand* command =
    new AddWidgetCommand(statusWidget);     // NOTE GPS widget stays onscreen so not removed in ::Exit()

    _display.Send(std::unique_ptr<IDisplayCommand>(command));

    WidgetPosition gpsText(TextX,TextY);
    TextWidget* textWidget = new TextWidget(UIWidgetIds::GPSInfo,"",gpsText,TextFont, MC_DATUM, TFT_WHITE, TFT_BLACK, TextZoom );
    command = new AddWidgetCommand(textWidget);
    _display.Send(std::unique_ptr<IDisplayCommand>(command));

}
void GPSLockState::Exit()
{
    RemoveWidgetCommand* remove = new RemoveWidgetCommand(UIWidgetIds::GPSInfo);
     _display.Send(std::unique_ptr<IDisplayCommand>(remove));   

}
