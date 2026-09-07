#pragma once

#include "weapons/Weapon.h"

#include <stdint.h>
#include <string>
#include <vector>

class WeaponConfig
{
public:
    WeaponConfig();
    void Init();

    const char* GetImageFilename(WeaponType type) const;
    const char* GetShootSampleFilename(WeaponType type) const;
    const char* GetClipInsertSampleFilename(WeaponType type) const;
    const char* GetCockingFilename(WeaponType type) const;
    const char* GetEmptyFilename(WeaponType type) const;   
    int GetPostMagTimeout(WeaponType type) const;
    int GetMagCapacity(WeaponType type) const;
    int GetMaxAmmoReserve(WeaponType type) const;
    int GetDefaultAmmoCount(WeaponType type) const;
    int GetMaxReload(WeaponType type) const;
    

private:
    struct WeaponDefinition
    {
        WeaponType type;
        std::string imageFilename;
        std::string shootSampleFilename;
        std::string clipInsertSampleFilename;
        std::string cockingFilename;
        std::string emptyFilename;
        int postMagTimeout;
        int magCapacity;
        int maxAmmoReserve;
        int defaultAmmoCount;
        int maxReload;
    };

    void PrintDef(WeaponDefinition& def);

    std::vector<WeaponDefinition> _weapons;

    bool Load();
    const WeaponDefinition* Find(WeaponType type) const;
};
