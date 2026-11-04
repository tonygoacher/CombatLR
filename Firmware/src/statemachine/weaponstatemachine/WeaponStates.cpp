


#include "weapons/Weapon.h"
#include "statemachine/weaponstatemachine/WeaponStates.h"
#include "statemachine/GenericEvent.h"
#include "IRLaserTag.h"
#include "ISounds.h"
#include "PBSwitch.h"
#include "PacedLED.h"
#include "SystemConfig.h"




/////////////////////////////////////////////////////////////////////////////////////////////////////////
// IDLE STATE
/////////////////////////////////////////////////////////////////////////////////////////////////////////
Idle::Idle(CombatLRServices& services, WeaponStateMachine& weaponStateMachine)
: WeaponBaseState(services,  weaponStateMachine)
{

}

void Idle::Entry()
{
    Serial.println("Entering IDLE state");
}

uint16_t Idle::Run()
{
    if(_weaponStateMachine.GetMagAmmo() == 0)
    {
        return WeaponStateMachine::WeaponStates::EmptyState;
    }

    if(_services.triggerSwitch.Pressed())
    {
        if(_services.weaponConfig.GetBuildUpTime(_weaponStateMachine.GetWeaponType()) == 0)
        {
            return WeaponStateMachine::WeaponStates::ShootState;
        }
        else
        {
             return WeaponStateMachine::WeaponStates::BuildUpState;
        }
    }

    if(_services.reloadSwitch.Pressed() && _weaponStateMachine.GetMagAmmo() != _weaponStateMachine.GetMagCapacity() && _weaponStateMachine.GetRemainingAmmo())
    {
        return WeaponStateMachine::WeaponStates::ReloadState;
    }

    return WeaponStateMachine::WeaponStates::IdleState;
}

void Idle::Exit()
{
    
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SHOOT STATE
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 Shoot::Shoot(CombatLRServices& services, WeaponStateMachine& weaponStateMachine)
 :  WeaponBaseState(services, weaponStateMachine)
 {

 }

void Shoot::Entry()
{
    Serial.println("Entering SHOOT state");

    _burstCount = _services.weaponConfig.GetBurst(_weaponStateMachine.GetWeaponType());

    DoShot(); 
}

void Shoot::Exit()
{
    _weaponStateMachine.CancelEvent(WeaponStateMachine::WeaponStateEvents::RateOfFireEvent);
}

void Shoot::DoShot()
{
    _services.sounds.PlayCached();

    LaserTagData tagData;

    tagData.team = _services.systemConfig.GetTeamId();
    tagData.playerId = _services.systemConfig.GetPlayerId();
    tagData.shotPowerIndex = _weaponStateMachine.GetWeaponType();     // This is an index into the weapons declared in weapons.json.
    _services.laserTag.SetIRPower(_services.weaponConfig.GetIRPowerLevel(_weaponStateMachine.GetWeaponType()));

    _services.laserTag.SendLasertagData(tagData);

    // The muzzle LED is time is run in the main loop so it only needs firing here
    int muzzleLEDTime = _services.weaponConfig.GetMuzzleLEDTime(_weaponStateMachine.GetWeaponType());
    if(muzzleLEDTime)
    {
        _services.muzzleLED.Fire(muzzleLEDTime);
    } 

    GenericEvent rofTimer(WeaponStateMachine::WeaponStateEvents::RateOfFireEvent);
    Serial.printf("Inter shot time is %d\n",  _services.weaponConfig.InterShotTime(_weaponStateMachine.GetWeaponType()));
    // Set the rate of fire time
    _weaponStateMachine.ScheduleEvent(rofTimer, _services.weaponConfig.InterShotTime(_weaponStateMachine.GetWeaponType()));

    if(_burstCount > 0) // If burstCount < 0, then this means full auto
    {
        _burstCount--;
    }

    _weaponStateMachine.TakeShot();
    Serial.printf("Weapon %d mag is %d\n", _weaponStateMachine.GetWeaponType(), _weaponStateMachine.GetMagAmmo());
    _weaponStateMachine.SetUpdated();

}

int Shoot::ProcessPostShot()
{
    // If the burst is exhausted or the mag is empty, just wait for the trigger to be released
    if(_burstCount == 0 || _weaponStateMachine.GetMagAmmo() == 0)
    {
        return WeaponStateMachine::WeaponStates::WaitForTriggerReleaseState;
    }

    // If there is any burst left, and the trigger is down and we have ammo then do another shot
    if(_burstCount !=0 &&  _weaponStateMachine.GetMagAmmo() && _services.triggerSwitch.Read())
    {
        DoShot();  
        return WeaponStateMachine::WeaponStates::ShootState;
    }

    // Wait for trigger release
    return WeaponStateMachine::WeaponStates::WaitForTriggerReleaseState;
}

uint16_t Shoot::Run()
{
    auto event = _weaponStateMachine.GetEvent();

    if(event != nullptr)
    {
        if(event->GetType() == WeaponStateMachine::WeaponStateEvents::RateOfFireEvent)
        {
            return ProcessPostShot();
        }
    }
    // Stay in Shoot state until we get an event
    return WeaponStateMachine::WeaponStates::ShootState;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
// WAIT_FOR_TRIGGER_RELEASE state
///////////////////////////////////////////////////////////////////////////////////////////////////////////

WaitForTriggerRelease::WaitForTriggerRelease(CombatLRServices& services, WeaponStateMachine& weaponStateMachine)
: WeaponBaseState(services, weaponStateMachine)
{

}

void WaitForTriggerRelease::Entry()
{
    Serial.println("Entering WAIT_FOR_TRIGGER_RELEASE state");
    _debounceTimeElapsed = false;
}



void WaitForTriggerRelease::Exit()
{
    _weaponStateMachine.CancelEvent(WeaponStateMachine::WeaponStateEvents::TriggerDebounce);
}


uint16_t WaitForTriggerRelease::Run()
{
    auto event = _weaponStateMachine.GetEvent();

    // Process a debounce event only if we were waiting for it.
    if (event != nullptr &&
        event->GetType() ==
            WeaponStateMachine::WeaponStateEvents::TriggerDebounce &&
        _debounceTimeElapsed)
    {
        _debounceTimeElapsed = false;

        // Trigger bounced back on: wait for another release.
        if (_services.triggerSwitch.RawRead())
        {
            return WeaponStateMachine::WeaponStates::WaitForTriggerReleaseState;
        }

        // Trigger is stably released.
        if (_weaponStateMachine.GetMagAmmo() > 0)
        {
            if(_services.weaponConfig.GetManualCocking(_weaponStateMachine.GetWeaponType()))
            {
                return WeaponStateMachine::WeaponStates::ManualCockingRequired;
            }
            return WeaponStateMachine::WeaponStates::IdleState;
        }
        return WeaponStateMachine::WeaponStates::EmptyState;
    }

    // Start the debounce timer on the first observed release.
    if (!_debounceTimeElapsed &&
        !_services.triggerSwitch.RawRead())
    {
        _debounceTimeElapsed = true;

        GenericEvent debounceTimer(
            WeaponStateMachine::WeaponStateEvents::TriggerDebounce);

        _weaponStateMachine.ScheduleEvent(
            debounceTimer, debounceTimeInms);
    }

    return WeaponStateMachine::WeaponStates::WaitForTriggerReleaseState;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Empty state
/////////////////////////////////////////////////////////////////////////////////////////////////////

Empty::Empty(CombatLRServices& services, WeaponStateMachine& weaponStateMachine)
: WeaponBaseState(services, weaponStateMachine)
{

}


void Empty::Entry()
{
    Serial.println("Entering EMPTY state");
}


 uint16_t Empty::Run()
 {
    if(_services.reloadSwitch.Pressed() && _weaponStateMachine.GetRemainingAmmo())
    {
        return WeaponStateMachine::WeaponStates::ReloadState;
    }

    if(_services.triggerSwitch.Pressed())   // Pressed only returns bool once for each press
    {
        _services.sounds.Play(_services.weaponConfig.GetEmptyFilename(_weaponStateMachine.GetWeaponType()));
    }

    return WeaponStateMachine::WeaponStates::EmptyState;
 }
 

 //////////////////////////////////////////////////////////////////////////////////////
 // RELOAD state
 /////////////////////////////////////////////////////////////////////////////////////
 
Reload::Reload(CombatLRServices& services, WeaponStateMachine& weaponStateMachine)
: WeaponBaseState(services, weaponStateMachine)
{

}


void Reload::Entry()
{
    Serial.println("Entering RELOAD state");

    const char *magInsert = _services.weaponConfig.GetClipInsertSampleFilename(_weaponStateMachine.GetWeaponType());
    if(magInsert != nullptr)
    {
        _services.sounds.Play(magInsert);
    }

    GenericEvent magInsertTimer(WeaponStateMachine::WeaponStateEvents::MagInsertTime);
    _weaponStateMachine.ScheduleEvent(magInsertTimer, _services.weaponConfig.GetInsertTime(_weaponStateMachine.GetWeaponType()));

}

uint16_t Reload::Run()
{
    auto event = _weaponStateMachine.GetEvent();
    if(event != nullptr)
    {
        if(event->GetType() == WeaponStateMachine::WeaponStateEvents::MagInsertTime)
        {   
            _weaponStateMachine.CompleteReload();   // Moves available ammo into magazine
            _weaponStateMachine.SetUpdated();
            return WeaponStateMachine::WeaponStates::CockingState;     
        }
    }
    return WeaponStateMachine::WeaponStates::ReloadState;   
}


/////////////////////////////////////////////////////////////////////////////////////////
// COCKING state. Pull back the cocking handle
/////////////////////////////////////////////////////////////////////////////////////////
Cocking::Cocking(CombatLRServices& services, WeaponStateMachine& weaponStateMachine)
: WeaponBaseState(services, weaponStateMachine)
{

}


void Cocking::Entry()
{
    Serial.println("Entering COCKING state");
    if(_weaponStateMachine.GetMagAmmo())
    {
        const char *cockingSample = _services.weaponConfig.GetCockingFilename(_weaponStateMachine.GetWeaponType());
        if(cockingSample != nullptr)
        {
            _services.sounds.Play(cockingSample);
        }

        GenericEvent magInsertTimer(WeaponStateMachine::WeaponStateEvents::CockingTime);
        _weaponStateMachine.ScheduleEvent(magInsertTimer, _services.weaponConfig.GetCockingTime(_weaponStateMachine.GetWeaponType()));
    }
    
}

uint16_t Cocking::Run()
{
    if(_weaponStateMachine.GetMagAmmo() == 0)
    {
        return WeaponStateMachine::WeaponStates::EmptyState;
    }

    auto event = _weaponStateMachine.GetEvent();
    if(event != nullptr && event->GetType() == WeaponStateMachine::WeaponStateEvents::CockingTime)
    {
        return WeaponStateMachine::WeaponStates::IdleState;
    }
    return WeaponStateMachine::WeaponStates::CockingState;
}            



//////////////////////////////////////////////////////////////////////////////
// BUILDUP
///////////////////////////////////////////////////////////////////////////

BuildUp::BuildUp(CombatLRServices& services, WeaponStateMachine& weaponStateMachine)
: WeaponBaseState(services, weaponStateMachine)
{

}

void BuildUp::Entry()
{
    int weaponType = _weaponStateMachine.GetWeaponType();

    _services.sounds.Play(_services.weaponConfig.GetBuildUpFilename(weaponType));
    GenericEvent buildUpTime(WeaponStateMachine::WeaponStateEvents::BuildUpTime);
    _weaponStateMachine.ScheduleEvent(buildUpTime, _services.weaponConfig.GetBuildUpTime(weaponType));
}    

uint16_t BuildUp::Run()
{
    auto event  = _weaponStateMachine.GetEvent();
    if(event != nullptr && event->GetType() == WeaponStateMachine::WeaponStateEvents::BuildUpTime)
    {
        return WeaponStateMachine::WeaponStates::ShootState;
    }

    if(!_services.triggerSwitch.Read())
    {
        return WeaponStateMachine::WeaponStates::IdleState;
    }

    return WeaponStateMachine::WeaponStates::BuildUpState;
}


void BuildUp::Exit()
{
    _services.sounds.Stop();
    _weaponStateMachine.CancelEvent(WeaponStateMachine::WeaponStateEvents::BuildUpTime);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
//MANUAL COCKING
//////////////////////////////////////////////////////////////////////////////////////////////////

ManualCocking:: ManualCocking(CombatLRServices& services, WeaponStateMachine& weaponStateMachine)
: WeaponBaseState(services, weaponStateMachine)
{
    
}

uint16_t ManualCocking::Run()
{
    if(_services.reloadSwitch.Pressed())
    {
        return WeaponStateMachine::WeaponStates::CockingState;
    }

    return WeaponStateMachine::WeaponStates::ManualCockingRequired;
}
