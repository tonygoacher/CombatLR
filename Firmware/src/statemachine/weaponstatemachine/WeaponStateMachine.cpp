#include "statemachine/weaponstatemachine/WeaponStateMachine.h"
#include "statemachine/weaponstatemachine/WeaponStates.h"
#include "ISounds.h"
#include "weapons/WeaponConfig.h"



WeaponStateMachine::WeaponStateMachine(WeaponType type, CombatLRServices& services)
:  StateEngine("WeaponStateMachine"), Weapon(type,services)
{
    _updated = false;
}





void WeaponStateMachine::Init()
{
    _states.insert(StateMapEntry(WeaponStateMachine::WeaponStates::IdleState, *new Idle(_services, *this)));
    _states.insert(StateMapEntry(WeaponStateMachine::WeaponStates::ShootState, *new Shoot(_services, *this)));
    _states.insert(StateMapEntry(WeaponStateMachine::WeaponStates::WaitForTriggerReleaseState, *new WaitForTriggerRelease(_services, *this)));
    _states.insert(StateMapEntry(WeaponStateMachine::WeaponStates::ReloadState, *new Reload(_services, *this)));
    _states.insert(StateMapEntry(WeaponStateMachine::WeaponStates::EmptyState, *new Empty(_services, *this)));
    _states.insert(StateMapEntry(WeaponStateMachine::WeaponStates::CockingState, *new Cocking(_services, *this)));
    _states.insert(StateMapEntry(WeaponStateMachine::WeaponStates::BuildUpState, *new BuildUp(_services, *this)));
    _states.insert(StateMapEntry(WeaponStateMachine::WeaponStates::ManualCockingRequired, *new ManualCocking(_services, *this)));    
    Start();
}



void WeaponStateMachine::Start()
{
    CancelAllEvents();

    // Preload the shot sample into the RAM cache
    const char *shotSample = _services.weaponConfig.GetShootSampleFilename(_type);
    Serial.println(shotSample);
    if(shotSample != nullptr)
    {
        _services.sounds.CacheSample(shotSample);
    }   

    _currentState = _states.find(WeaponStates::CockingState);
    if(_currentState != _states.end())
    {
        _currentState->second.Entry();
    }
    else
    {
        Serial.printf("WeaponStateMachine::Start unable to find start state");
    }
}


void WeaponStateMachine::Equip()
{
    Start();
}


void WeaponStateMachine::AddAmmo(uint16_t addAmmo)
{


    _services.sounds.Play("/sounds/ampick.wav");   
    _reserveAmmo += addAmmo;

}


const char * const WeaponStateMachine::GetFilename() 
{
    return _services.weaponConfig.GetImageFilename(_type);  
}



bool WeaponStateMachine::Updated()
{
    bool current = _updated;
    _updated = false;
    return current;
}
