#pragma once

#include "weapons/Weapon.h"
#include "Pacer.h"



class Texture;
class PBSwitch;
class IRLaserTag;



class AssaultRifle : public Weapon
{
public:
    AssaultRifle(WeaponType type, WeaponConfig& weaponConfig,CombatLRServices& services);
    
    void AddAmmo(uint16_t addAmmo) override;
    const char * const GetFilename() override;
    void Equip();

protected:
    virtual bool CockingAction();
    virtual void MagInsertAction();
    virtual void EmptyAction();
    virtual void Shoot();
    virtual void ShootWait();  // While shooting, use this for inter round dely
    virtual void StartReload();
    virtual bool IdleAction();     // Check the trigger and reload, start the shot
    virtual void EquippingAction();
    virtual bool BuildUpAction();

    const char * const GetShotFilename();
    const char * const GetReloadFilename();
    const char * const GetEmptyFilename();
    const char * const GetMagInsertFilename();

private:
    Pacer _reloadPacer;
    Pacer _rpmPacer;
    int _burst;
    int _shotsTaken;
    
    enum class ARState
    {
        Idle,
        BuildUp,
        Shoot,
        ShootWait,
        Empty,
        StartReload,
        MagInsert,
        Cocking,
        Equipping
    };

    ARState _state;
};