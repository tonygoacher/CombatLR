
#include "weapons/WeaponManager.h"

#include "widgets/TextWidget.h"
#include "widgets/UIWidgetIds.h"
#include "DisplayManager.h"
#include "AddWidgetCommand.h"
#include "SetTextCommand.h"
#include "RemoveWidgetCommand.h"








WeaponManager::WeaponManager(StateData & stateData)
: _stateData(stateData)
{

}



void WeaponManager::Run()
{
    if(_currentWeapon != _weapons.end())
    {
        _currentWeapon->second.Run();
    }
    
}




void WeaponManager::Init()
{
    TextWidget* loadout =  new TextWidget(UIWidgetIds::Loadout, "00/00", WidgetPosition(TextX, TextY),TextFont,MC_DATUM, TFT_RED, TFT_BLACK, TextZoom);
     _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(
                    loadout)));

}



void WeaponManager::Reset()
{
      _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new RemoveWidgetCommand(
                   UIWidgetIds::Loadout)));
                    
      _weapons.clear();
}


