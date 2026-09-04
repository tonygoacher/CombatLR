
#include "/weapons/Weapon.h"
#include "TextureManager.h"



    Weapon::Weapon( uint16_t ammo, uint16_t magSize, IRLaserTag& laserTag, PBSwitch& trigger, PBSwitch& reload, ISounds& sounds)
    :  _laserTag(laserTag), _reserveAmmo(ammo), _magCapacity(magSize), _trigger(trigger), _reload(reload), _sounds(sounds)
    {
        CompleteReload();
    };