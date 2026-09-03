#pragma once

#include "weapons/Weapon.h"
#include "Pacer.h"


class Texture;
class PBSwitch;
class IRLaserTag;



class AssaultRifle : public Weapon
{
public:
    AssaultRifle( uint16_t ammo, uint16_t magSize, IRLaserTag& laserTag, PBSwitch& trigger, PBSwitch& reload, ISounds& sounds);
    
    void Run() override;
    void AddAmmo(uint16_t addAmmo) override;
    void Start() override;
    void Stop() override;
    WeaponType GetWeaponType() override { return WeaponType::AssaultRifle;}
private:
    Pacer _reloadPacer;
    Pacer _rpmPacer;
    
    enum class ARState
    {
        Idle,
        Shooting,
        MagInsert,
        Cocking
    };

    ARState _state;
};