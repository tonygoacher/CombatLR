
#include "weapons/WeaponManager.h"
#include "widgets/UIWidget.h"

#include "widgets/TextWidget.h"
#include "widgets/UIWidgetIds.h"
#include "graphics/DisplayManager.h"
#include "graphics/AddWidgetCommand.h"
#include "graphics/SetTextCommand.h"
#include "graphics/RemoveWidgetCommand.h"
#include "PBSwitch.h"
#include "weapons/AssaultRifle.h"








WeaponManager::WeaponManager(CombatLRServices & stateData)
: _stateData(stateData), _selectPacer(false)
{
    _state = WMStates::Normal;
    _selectPacer.SetPace(SelectTimeout);
    _displayWeapon = _weapons.end();
    _currentWeapon = _weapons.end();
}





void WeaponManager::Run()
{
    if(_currentWeapon != _weapons.end())
    {
        if(_state == WMStates::Normal)
        {

                if(_currentWeapon->second->Run())
                {
                    ShowLoadout(_currentWeapon);
                }


            // Left or right button pressed and at least one weapon, move to show mode
            if(_stateData.leftSwitch.Pressed() || _stateData.rightSwitch.Pressed() && _weapons.size() > 1)
            {
                _state = WMStates::Select;
                _displayWeapon = _currentWeapon;
                ShowSelectWeapon();
                _selectPacer.PacerReset();
            }
        }

        
        if(_state == WMStates::Select && _weapons.size() > 1)
        {
            bool actionTaken = false;
            if(_stateData.leftSwitch.Pressed())
            {
                _selectPacer.PacerReset();

                actionTaken = true;
                if(_displayWeapon == _weapons.begin())
                {
                    _displayWeapon = std::prev(_weapons.end());
                }
                else
                {
                    _displayWeapon = std::prev(_displayWeapon);
                }
            }

            if(_stateData.rightSwitch.Pressed())
            {
                _selectPacer.PacerReset();
            
                actionTaken = true;
                if(std::next(_displayWeapon) == _weapons.end())
                {
                    _displayWeapon = _weapons.begin();
                }
                else
                {
                    _displayWeapon = std::next(_displayWeapon);
                }
            }

            if(actionTaken)
            {
                ShowSelectWeapon();
            }

            if(_stateData.triggerSwitch.Pressed())
            {
                _stateData.displayManager.Send(std::unique_ptr<IDisplayCommand>(new RemoveWidgetCommand(UIWidgetIds::SelectWeapon))); 
                if(_currentWeapon->first != _displayWeapon->first)
                {
                    _currentWeapon = _displayWeapon;
                
                    _currentWeapon->second->Equip();
                }                
                ShowLoadout(_currentWeapon);
                _state = WMStates::Normal;

            }


        }
        if(_selectPacer.Pace())
        {
            _state = WMStates::Normal;
            _stateData.displayManager.Send(std::unique_ptr<IDisplayCommand>(new RemoveWidgetCommand(UIWidgetIds::SelectWeapon))); 
        }
    }        
}




void WeaponManager::Init()
{
    TextWidget* loadout =  new TextWidget(UIWidgetIds::AmmoDisplay, "00 :00", WidgetPosition(TextX, TextY),TextFont,MC_DATUM, TFT_WHITE, TFT_BLACK, TextZoom);
     _stateData.displayManager.Send(
            std::unique_ptr<IDisplayCommand>(
                new AddWidgetCommand(
                    loadout)));

}


void WeaponManager::ShowLoadout(WeaponMap::iterator& weapon)
{
    uint16_t allAmmo = weapon->second->GetRemainingAmmo();
    uint16_t magAmmo = weapon->second->GetMagAmmo();
    char data[16];
    sprintf(data, "%02d :%03d", magAmmo, allAmmo);

    _stateData.displayManager.Send(std::unique_ptr<IDisplayCommand>(
        new SetTextCommand(UIWidgetIds::AmmoDisplay, data)));

}


void WeaponManager::ShowSelectWeapon()
{
    _stateData.displayManager.Send(std::unique_ptr<IDisplayCommand>(
        new RemoveWidgetCommand(UIWidgetIds::SelectWeapon)));   
    
    UIWidget* gun = new UIWidget(UIWidgetIds::SelectWeapon, _displayWeapon->second->GetFilename(), WidgetPosition(160-64,120-32));  
    
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






bool WeaponManager::InsertWeapon(std::pair<WeaponMap::iterator, bool>& insertResult)
{
        bool result = false;
        if(insertResult.second)
        {
            _currentWeapon = _displayWeapon = insertResult.first;
            ShowSelectWeapon();
            Serial.printf("Insert w %d\n", static_cast<int>(_currentWeapon->second->GetWeaponType()));
            _currentWeapon->second->Start();
            ShowLoadout(_currentWeapon);
            result = true;
            _selectPacer.PacerReset();
        }
        else
        {
            Serial.println("Weapon insert failed");
        }
        return result;
}




void WeaponManager::AddWeapon(WeaponType type)
{
    Serial.print("Adding weapon type "); Serial.println(static_cast<int>(type));
    switch(type)
    {
        case WeaponType::AssaultRifle:
        {   

            std::pair<WeaponMap::iterator, bool> result;
            result = _weapons.emplace(type, std::make_shared<AssaultRifle>( type, _stateData.weaponConfig,  _stateData, 3));    
            InsertWeapon(result);

        }
        break;

        case WeaponType::AssaultRifleFullAuto:
        {   
                std::pair<WeaponMap::iterator, bool> result;
                result = _weapons.emplace(type, std::make_shared<AssaultRifle>( type,  _stateData.weaponConfig, _stateData, -1));    
                InsertWeapon(result);
        }
        break;

        case WeaponType::Blaster:
        {   
                std::pair<WeaponMap::iterator, bool> result;
                result = _weapons.emplace(type, std::make_shared<AssaultRifle>( type,  _stateData.weaponConfig, _stateData, -1));    
                InsertWeapon(result);
        }
        break;
    };
}



void WeaponManager::AddAmmo(WeaponType type, uint16_t ammo)
{
    Serial.println("Adding ammo");
    WeaponMap::iterator weapon = _weapons.find(type);
    if(weapon != _weapons.end())
    {
        weapon->second->AddAmmo(ammo);
    }
}




const WeaponManager::WeaponMap& WeaponManager::GetWeaponMap()
{
    return _weapons;
}
