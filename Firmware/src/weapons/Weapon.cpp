
#include "weapons/Weapon.h"
#include "TextureManager.h"



Weapon::Weapon( uint16_t ammo, uint16_t magSize, CombatLRServices& services)
:   _reserveAmmo(ammo), _magCapacity(magSize), _services(services)
{
    CompleteReload();
};

