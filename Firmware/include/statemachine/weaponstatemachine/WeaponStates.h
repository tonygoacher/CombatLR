#pragma once

#include "statemachine/weaponstatemachine/WeaponStateMachine.h"
#include "weapons/WeaponManager.h"
#include "statemachine/Services.h"

class WeaponBaseState : public State
{
public:    
    WeaponBaseState(CombatLRServices& services, WeaponStateMachine& weaponStateMachine)
    : _services(services), _weaponStateMachine(weaponStateMachine)
    {

    }
 

protected:
    CombatLRServices&    _services;
    WeaponStateMachine& _weaponStateMachine;

};


class Idle : public WeaponBaseState
{
public:
    Idle(CombatLRServices& services, WeaponStateMachine& weaponStateMachine);
    void Entry() override;
    void Exit() override;
    uint16_t Run() override;
};

//////////////////////////////////////////////////////////////////////////////////////

class Shoot : public WeaponBaseState
{
public:
    Shoot(CombatLRServices& services, WeaponStateMachine& weaponStateMachine);

    
    void Entry() override;
    uint16_t Run() override;
    void Exit() override;
private:
    int ProcessPostShot();
    void DoShot();
    int _burstCount;
};


/////////////////////////////////////////////////////////////////////////////////////
class WaitForTriggerRelease  : public WeaponBaseState
{
public:
    WaitForTriggerRelease(CombatLRServices& services, WeaponStateMachine& weaponStateMachine);

    
    void Entry()override;
    uint16_t Run() override;
    void Exit() override; 

private:
    static const uint16_t debounceTimeInms = 20;    
    bool   _debounceTimeElapsed;
};

/////////////////////////////////////////////////////////////////////////////////////

class Empty  : public WeaponBaseState
{
public:
    Empty(CombatLRServices& services, WeaponStateMachine& weaponStateMachine);

    
    void Entry() override;
    uint16_t Run() override;
    void Exit(){} ; 
};    

/////////////////////////////////////////////////////////////////////////////////////

class Reload  : public WeaponBaseState
{
public:
    Reload(CombatLRServices& services, WeaponStateMachine& weaponStateMachine);

    
    void Entry() override;
    uint16_t Run() override;
    void Exit(){} ; 
};  

//////////////////////////////////////////////////////////////////////////////////////

class Cocking  : public WeaponBaseState
{
public:
    Cocking(CombatLRServices& services, WeaponStateMachine& weaponStateMachine);

    
    void Entry() override;
    uint16_t Run() override;
    void Exit() ; 
};  

/////////////////////////////////////////////////////////////////////////////

class BuildUp  : public WeaponBaseState
{
public:
    BuildUp(CombatLRServices& services, WeaponStateMachine& weaponStateMachine);

    
    void Entry() override;
    uint16_t Run() override;
    void Exit() override;
};  

/////////////////////////////////////////////////////////////////////////////

class ManualCocking  : public WeaponBaseState
{
public:
    ManualCocking(CombatLRServices& services, WeaponStateMachine& weaponStateMachine);

    
    void Entry() override;
    uint16_t Run() override;
    void Exit() override{}; 
};  


/////////////////////////////////////////////////////////////////////////////////////


class CountedReload  : public WeaponBaseState
{
public:
    CountedReload(CombatLRServices& services, WeaponStateMachine& weaponStateMachine);

    
    void Entry() override;
    uint16_t Run() override;
    void Exit() override; 
private:
    void  RoundInsert();
};  