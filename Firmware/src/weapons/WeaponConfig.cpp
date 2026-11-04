#include "weapons/WeaponConfig.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <utility>

namespace
{
    constexpr const char* WeaponConfigFilename =
        "/weapons/weapons.json";
}

WeaponConfig::WeaponConfig()
{

}


void WeaponConfig::PrintDef(WeaponDefinition& def)
{
    Serial.print("Type:"); Serial.println(static_cast<int>(def.type));
    Serial.printf("Name             %s\n",def.name.c_str());
    Serial.printf("Insert sample:   %s\n",def.clipInsertSampleFilename.c_str());
    Serial.printf("Cocking sample:  %s\n",def.cockingFilename.c_str());
    Serial.printf("Empty sample:    %s\n",def.emptyFilename.c_str());
    Serial.printf("Image:           %s\n",def.imageFilename.c_str());
    Serial.printf("Shoot sample:    %s\n",def.shootSampleFilename.c_str());
    Serial.printf("Build Up Sample %s\n", def.buildUpFilename.c_str());
    Serial.printf("Insert Clip Time:%d\n", def.insertClipTime);
    Serial.printf("Cocking Time:    %d\n", def.cockingTime);
    Serial.printf("Mag capacity:    %d\n",def.magCapacity);
    Serial.printf("Max ammo rserve: %d\n",def.maxAmmoReserve);
    Serial.printf("Default ammo:    %d\n",def.defaultAmmoCount);
    Serial.printf("Max reload:      %d\n", def.maxReload);
    Serial.printf("Burst:           %d\n", def.burst);
    Serial.printf("Inter shot time  %d\n", def.interShotTime);    
    Serial.printf("Build up time    %d\n", def.buildUpTime);
    Serial.printf("Post Build time  %d\n", def.postBuildUpShotTime);
    Serial.printf("Shot Power       %d\n", def.shotPower);    
    Serial.printf("Manual Cocking   %d\n", def.manualCocking);  

}


bool WeaponConfig::Load()
{
    _weapons.clear();

    File file = LittleFS.open(
        WeaponConfigFilename,
        FILE_READ);

    if (!file)
    {
        Serial.printf(
            "WeaponConfig: failed to open %s\n",
            WeaponConfigFilename);

        return false;
    }

    JsonDocument document;

    const DeserializationError error =
        deserializeJson(
            document,
            file);

    file.close();

    if (error)
    {
        Serial.printf(
            "WeaponConfig: JSON parse failed: %s\n",
            error.c_str());

        return false;
    }

    JsonArray weapons =
        document["weapons"].as<JsonArray>();

    if (weapons.isNull())
    {
        Serial.println(
            "WeaponConfig: missing weapons array");

        return false;
    }



    for (JsonObject weaponJson : weapons)
    {
        if (!weaponJson["type"].is<int>())
        {
            Serial.println(
                "WeaponConfig: weapon entry missing type");

            continue;
        }

        WeaponDefinition definition;

        definition.type =
            static_cast<WeaponType>(
                weaponJson["type"].as<int>());

        definition.imageFilename =
            weaponJson["image"] | "";

        definition.shootSampleFilename =
            weaponJson["shoot"] | "";

        definition.name =
            weaponJson["name"] | "";

        definition.clipInsertSampleFilename =
            weaponJson["clipInsert"] | "";

        definition.cockingFilename =
            weaponJson["cocking"] | "";

        definition.emptyFilename =
            weaponJson["empty"] | "";

        definition.buildUpFilename =
            weaponJson["buildUp"] | "";

        definition.burst =
            weaponJson["burst"] | 1;
        if(definition.burst == 0)
        {
            definition.burst = 1;
        }

        definition.manualCocking =
            weaponJson["manualCocking"] == 1;


            
        int roundsPerMin =
            weaponJson["rpm"] | 0;

        if(roundsPerMin == 0 || definition.burst == 1)
        {
            roundsPerMin = msInOneMinute;       // This makes the inter shot time 1ms. This would be used of there was no rpm field. Normally for a none auto weapon
        }

        definition.interShotTime = msInOneMinute / roundsPerMin;         
   

        if(definition.interShotTime < 50)   // IR pulse take ~ 32ms so need to clamp this
        {
            definition.interShotTime  = 50;
        }

        definition.insertClipTime =
            weaponJson["insertClipTime"] | 0;

        definition.cockingTime =  
            weaponJson["cockingTime"] | 0;            


        definition.buildUpTime = 
            weaponJson["buildUpTime"] | 0;        

        definition.postBuildUpShotTime = 
            weaponJson["postBuildUpShotTime"] | 0;               

        definition.muzzleLedTime = 
            weaponJson["muzzleLED"] | 0;   

        definition.magCapacity = weaponJson["magCapacity"] | 0;

        definition.maxAmmoReserve = weaponJson["maxAmmoReserve"] | 0;

        definition.defaultAmmoCount = weaponJson["defaultAmmoCount"] | 0;

        definition.maxReload = weaponJson["maxReload"] | 0;

        definition.shotPower = weaponJson["shotPower"] | 0; // This is the power level that will be sent in the IR packet

        definition.irPowerLevel = ConvertToPowerLevel(weaponJson["irPower"] | 0);

        PrintDef(definition);
        _weapons.push_back(
            std::move(definition));
    }

    Serial.printf(
        "WeaponConfig: loaded %u weapon definitions\n",
        static_cast<unsigned>(_weapons.size()));

    return true;
}

IRPowerLevel WeaponConfig::ConvertToPowerLevel(int powerLevel) const
{
    IRPowerLevel irPowerLevel = IRPowerLevel::IRPowerLow;

    switch(powerLevel)
    {
        case    0:
        {
            irPowerLevel = IRPowerLevel::IRPowerLow;
        }
        break;

        case    1:
        {
            irPowerLevel = IRPowerLevel::IRPowerMedium;
        }
        break;

        case    2:
        {
            irPowerLevel = IRPowerLevel::IRPowerHigh;
        }
        break;

        default:
        {
            Serial.printf("WeaponConfig Invalid IR power level %d\n", powerLevel);
        }            
       
    }

    return irPowerLevel;
}


const WeaponConfig::WeaponDefinition*
WeaponConfig::Find(WeaponType type) const
{
    Serial.printf("Finding weapon %d\n",static_cast<int>(type));
    for (const WeaponDefinition& weapon : _weapons)
    {
        if (weapon.type == type)
            return &weapon;
    }
    Serial.println("Not found");
    return nullptr;
}

const char*
WeaponConfig::GetImageFilename(
    WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? nullptr : weapon->imageFilename.c_str();
}

const char*
WeaponConfig::GetShootSampleFilename(
    WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? nullptr : weapon->shootSampleFilename.c_str();
}

const char*
WeaponConfig::GetClipInsertSampleFilename(
    WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? nullptr : weapon->clipInsertSampleFilename.c_str();
}

const char*
WeaponConfig::GetCockingFilename(
    WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? nullptr : weapon->cockingFilename.c_str();
}

const char*
WeaponConfig::GetEmptyFilename(
    WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? nullptr : weapon->emptyFilename.c_str();
}

int WeaponConfig::GetInsertTime(
    WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 0 : weapon->insertClipTime;
}


int WeaponConfig::GetCockingTime(
    WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 0 : weapon->cockingTime;
}


void WeaponConfig::Init()
{
    Serial.println("Weapon config init");
    Load();
}



int WeaponConfig::GetMagCapacity(WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 0 : weapon->magCapacity;
}


int WeaponConfig::GetMaxAmmoReserve(WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 0 : weapon->maxAmmoReserve;
}


int WeaponConfig::GetDefaultAmmoCount(WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 0 : weapon->defaultAmmoCount;
}


int WeaponConfig::GetMaxReload(WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 0 : weapon->maxReload;
}


int WeaponConfig::GetBurst(WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 1 : weapon->burst;
}


int WeaponConfig::InterShotTime(
    WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 1000 : weapon->interShotTime;
}


const char* WeaponConfig::GetName(WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? nullptr : weapon->name.c_str();
}



int WeaponConfig::GetNumWeapons()
{
    return _weapons.size();
}



int WeaponConfig::GetBuildUpTime(WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 0 : weapon->buildUpTime;
}




const char* WeaponConfig::GetBuildUpFilename(WeaponType type)
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? nullptr : weapon->buildUpFilename.c_str();   
}



int WeaponConfig::GetPostBuildUpShotTime(WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 0 : weapon->postBuildUpShotTime;   
}



int WeaponConfig::GetShotPower(WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 0 : weapon->shotPower;  
}



int WeaponConfig::GetMuzzleLEDTime(WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? 0 : weapon->muzzleLedTime;  
}



IRPowerLevel WeaponConfig::GetIRPowerLevel(WeaponType type) const
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? IRPowerLevel::IRPowerLow : weapon->irPowerLevel;
}




bool WeaponConfig::GetManualCocking(WeaponType type)
{
    const WeaponDefinition* weapon = Find(type);
    return weapon == nullptr ? false : weapon->manualCocking;
}
