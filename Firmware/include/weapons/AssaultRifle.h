#pragma once

#include "weapons/Weapon.h"
#include "Pacer.h"



class Texture;
class PBSwitch;
class IRLaserTag;



class AssaultRifle : public Weapon
{
public:
    AssaultRifle( uint16_t ammo, uint16_t magSize, CombatLRServices& services, int burst);
    
    bool Run() override;
    void AddAmmo(uint16_t addAmmo) override;
    void Start() override;
    void Stop() override;
    WeaponType GetWeaponType() override { return WeaponType::AssaultRifle;}
    const char * const GetFilename() override;
    void Equip();

protected:
    virtual bool CockingAction();
    virtual void MagInsertAction();
    virtual void EmptyAction();
    virtual void ShootingAction();  // While shooting, use this for inter round dely
    virtual bool IdleAction();     // Check the trigger and reload, start the shot
    virtual void EquippingAction();

private:
    Pacer _reloadPacer;
    Pacer _rpmPacer;
    int _burst;
    int _shotsTaken;
    
    enum class ARState
    {
        Idle,
        Shooting,
        Empty,
        MagInsert,
        Cocking,
        Equipping
    };

    ARState _state;
};