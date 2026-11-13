#pragma once



#include "statemachine/StateEngine.h"
#include "statemachine/Services.h"
#include "weapons/Weapon.h"




class WeaponStateMachine : public StateEngine, public Weapon
{
public:
    WeaponStateMachine(WeaponType type,CombatLRServices& services);
    void Start() override;
    void Init();
    void Equip() override;
    void Unslung() override{};
    void AddAmmo(uint16_t addAmmo) override;
    void SetAmmo(uint16_t ammo) override;
    const char * const GetFilename() override;
    bool Updated();
    void SetUpdated() { _updated = true;}

    enum WeaponStates
    { 
        IdleState,
        BuildUpState,
        ShootState,
        WaitForTriggerReleaseState,
        ManualCockingRequired,
        CockingState,
        EmptyState,
        ReloadState,
        CountedReloadState,
    };

    enum WeaponStateEvents
    {
        RateOfFireEvent, 
        TriggerDebounce,       
        MagInsertTime, 
        CockingTime,
        BuildUpTime,
        CountedReloadRoundInsertTime,
    };
private:
    bool _updated;    

};


