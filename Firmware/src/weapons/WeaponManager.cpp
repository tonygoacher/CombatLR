
#include "weapons/WeaponManager.h"
#include "widgets/UIWidget.h"

#include "widgets/TextWidget.h"
#include "widgets/UIWidgetIds.h"
#include "DisplayManager.h"
#include "AddWidgetCommand.h"
#include "SetTextCommand.h"
#include "RemoveWidgetCommand.h"
#include "PBSwitch.h"
#include "weapons/AssaultRifle.h"








WeaponManager::WeaponManager(CombatLRServices & stateData)
: _stateData(stateData)
{
    _state = WMStates::Normal;
}





void WeaponManager::Run()
{
    
    if(_state == WMStates::Normal)
    {
        if(_currentWeapon != _weapons.end())
        {
            if(_currentWeapon->second.Run())
            {
                ShowLoadout(_currentWeapon);
            }
        }

        // Left or right button pressed and at least one weapon, move to show mode
        if(_stateData.leftSwitch.Pressed() || _stateData.rightSwitch.Pressed() && _weapons.size())
        {
            _state = WMStates::Select;
            _displayWeapon = _currentWeapon;
            ShowSelectWeapon();
        }
    }
    else
    {

    }      

}




void WeaponManager::Init()
{
    TextWidget* loadout =  new TextWidget(UIWidgetIds::AmmoDisplay, "00/00", WidgetPosition(TextX, TextY),TextFont,MC_DATUM, TFT_RED, TFT_BLACK, TextZoom);
     _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(
                    loadout)));

}


void WeaponManager::ShowLoadout(std::map<WeaponType, Weapon&>::iterator& weapon)
{
    uint16_t allAmmo = weapon->second.GetRemainingAmmo();
    uint16_t magAmmo = weapon->second.GetMagAmmo();
    char data[16];
    sprintf(data, "%d:%d", magAmmo, allAmmo);

    _stateData.displayManager.Send(std::unique_ptr<IDisplayCommand>(
        new SetTextCommand(UIWidgetIds::AmmoDisplay, data)));

}


void WeaponManager::ShowSelectWeapon()
{
    _stateData.displayManager.Send(std::unique_ptr<IDisplayCommand>(
        new RemoveWidgetCommand(UIWidgetIds::SelectWeapon)));   
    
    UIWidget* gun = new UIWidget(UIWidgetIds::SelectWeapon, _displayWeapon->second.GetFilename(), WidgetPosition(64,100));  
    
    _stateData.displayManager.Send(std::unique_ptr<IDisplayCommand>(
            new AddWidgetCommand(gun)));
    ShowLoadout(_displayWeapon);            
}


void WeaponManager::Reset()
{
      _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new RemoveWidgetCommand(
                   UIWidgetIds::SelectWeapon)));
                    
      _weapons.clear();
}



void WeaponManager::AddWeapon(WeaponType type, uint16_t initialAmmo)
{
    Serial.print("Adding weapon type "); Serial.println(static_cast<int>(type));
    switch(type)
    {
        case WeaponType::AssaultRifle:
        {   
             std::map<WeaponType, Weapon&>::iterator w =_weapons.find(type);
            if(w != _weapons.end())    
            {
                w->second.AddAmmo(initialAmmo);
            }
            else
            {
                AssaultRifle ar(initialAmmo, 30, _stateData);
                std::pair<std::map<WeaponType, Weapon&>::iterator, bool> result;
                result = _weapons.insert(std::pair<WeaponType, Weapon&>(type,ar));
                if(result.second)
                {
                    _currentWeapon = result.first;
                }
                else
                {
                    Serial.printf("Failed to insert weapon %d\n", type);
                }
            }
        }
        break;
    };
}



void WeaponManager::AddAmmo(WeaponType type, uint16_t ammo)
{

}
