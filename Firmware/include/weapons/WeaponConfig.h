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
    const char* GetName(WeaponType type) const;
    const char* GetBuildUpFilename(WeaponType type);

    int GetInsertTime(WeaponType type) const;
    int GetCockingTime(WeaponType type) const;
    int GetMagCapacity(WeaponType type) const;
    int GetMaxAmmoReserve(WeaponType type) const;
    int GetDefaultAmmoCount(WeaponType type) const;
    int GetMaxReload(WeaponType type) const;
    int GetBurst(WeaponType type) const;
    int GetBuildUpTime(WeaponType type) const;
    int GetPostBuildUpShotTime(WeaponType type) const;
    int GetNumWeapons();
    

private:
    struct WeaponDefinition
    {
        WeaponType type;
        std::string name;
        std::string imageFilename;
        std::string shootSampleFilename;
        std::string clipInsertSampleFilename;
        std::string cockingFilename;
        std::string emptyFilename;
        std::string buildUpFilename;
        int insertClipTime;
        int cockingTime;
        int magCapacity;
        int maxAmmoReserve;
        int defaultAmmoCount;
        int maxReload;
        int burst;
        int buildUpTime;
        int postBuildUpShotTime;
    };

    void PrintDef(WeaponDefinition& def);

    std::vector<WeaponDefinition> _weapons;

    bool Load();
    const WeaponDefinition* Find(WeaponType type) const;
};
