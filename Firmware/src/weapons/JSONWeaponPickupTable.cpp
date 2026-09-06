
#include "weapons/JSONWeaponPickupTable.h"

#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

JsonWeaponPickupTable::JsonWeaponPickupTable(
    const char* filename)
{
    Serial.println("JsonWeaponPickupTable");
    Load(filename);
    Serial.println("Done");
}

bool JsonWeaponPickupTable::Load(
    const char* filename)
{
    _weapons.clear();

    File file = LittleFS.open(
        filename,
        "r");

    if (!file)
    {
        Serial.printf(
            "Unable to open weapon pickup table: %s\n",
            filename);

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
            "Failed to load weapon pickup table: %s\n",
            error.c_str());

        return false;
    }

    JsonArray weapons =
        document["weaponPickups"].as<JsonArray>();

    if (weapons.isNull())
    {
        Serial.println(
            "Weapon pickup table has no weaponPickups array");

        return false;
    }

    for (JsonObject entry : weapons)
    {
        const int type =
            entry["type"] | -1;

        if (type < 0)
            continue;

        WeaponPickupDefinition definition;

        definition.type =
            static_cast<WeaponType>(type);

        definition.pickupChance =
            entry["chance"] | 0;

        definition.minimumProgress =
            entry["minimumProgress"] | 0;

        _weapons.push_back(
            definition);
    }

    Serial.printf(
        "Loaded %d weapon pickup definitions\n",
        static_cast<int>(_weapons.size()));

    return true;
}

uint8_t JsonWeaponPickupTable::GetPickupChance(
    WeaponType type,
    uint8_t gameProgress) const
{
    for (const WeaponPickupDefinition& definition :
         _weapons)
    {
        if (definition.type != type)
            continue;

        if (gameProgress < definition.minimumProgress)
            return 0;

        return definition.pickupChance;
    }

    return 0;
}
