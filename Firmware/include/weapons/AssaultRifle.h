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