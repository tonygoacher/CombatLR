#include "GamePlan.h"

#include <ArduinoJson.h>
#include <LittleFS.h>


bool GamePlan::Load(const char* filename)
{
    _version = 0;
    _homeLatitude = 0.0;
    _homeLongitude = 0.0;
    _valid = false;

    if (filename == nullptr)
        return false;

    /*
     * Use the application's filesystem. The gameplan is expected to
     * have been mounted before Load() is called.
     *
     * This initial implementation uses LittleFS. If CombatLR is using
     * another filesystem for its map assets, this can be switched
     * without changing the GamePlan interface.
     */
    File file = LittleFS.open(filename, FILE_READ);

    if (!file)
    {
        Serial.printf(
            "GamePlan: failed to open %s\n",
            filename);

        return false;
    }

    JsonDocument document;

    const DeserializationError error =
        deserializeJson(document, file);

    file.close();

    if (error)
    {
        Serial.printf(
            "GamePlan: JSON parse failed: %s\n",
            error.c_str());

        return false;
    }

    /*
     * version
     */
    if (!document["version"].is<int>())
    {
        Serial.println(
            "GamePlan: missing or invalid version");

        return false;
    }

    /*
     * home
     */
    JsonObject home =
        document["home"].as<JsonObject>();

    if (home.isNull())
    {
        Serial.println(
            "GamePlan: missing home");

        return false;
    }

    if (!home["latitude"].is<double>() ||
        !home["longitude"].is<double>())
    {
        Serial.println(
            "GamePlan: invalid home coordinates");

        return false;
    }

    _version =
        document["version"].as<int>();

    _homeLatitude =
        home["latitude"].as<double>();

    _homeLongitude =
        home["longitude"].as<double>();

    _valid = true;

    JsonArray zones =
    document["zones"].as<JsonArray>();

if (zones.isNull())
{
    Serial.println(
        "GamePlan: missing zones");

    return false;
}

for (JsonObject zoneJson : zones)
{
    Zone zone;

    zone.eastMetres =
        zoneJson["eastMetres"].as<double>();

    zone.northMetres =
        zoneJson["northMetres"].as<double>();

    zone.radiusMetres =
        zoneJson["radiusMetres"].as<double>();

    zone.startTime =
        zoneJson["startTime"].as<uint32_t>();

    zone.lifespanSeconds =
        zoneJson["lifespanSeconds"].as<uint32_t>();

    zone.id = ZoneWidgetIdBase + _zones.size(); // Zone id's start at ZoneWidgetIdBase

    JsonArray pickups =
        zoneJson["pickups"].as<JsonArray>();

    if (!pickups.isNull())
    {
        for (JsonObject pickupJson : pickups)
        {
            Pickup pickup;

            pickup.eastMetres =
                pickupJson["eastMetres"].as<double>();

            pickup.northMetres =
                pickupJson["northMetres"].as<double>();

           
            int intType = pickupJson["type"].as<int>();
            pickup.type = static_cast<PickupTypes>(intType);
            
            pickup.spawnTimeSeconds =
                pickupJson["spawnTimeSeconds"].as<uint32_t>();

            pickup.lifespanSeconds =
                pickupJson["lifespanSeconds"].as<uint32_t>();

            pickup.id =
                pickupJson["id"].as<int>();

            zone.pickups.emplace(
                pickup.id,
                pickup);
        }
    }

    _zones.push_back(
        std::move(zone));
}

    Serial.printf(
        "GamePlan: loaded version %d\n",
        _version);

    Serial.printf(
        "GamePlan: home %.8f, %.8f\n",
        _homeLatitude,
        _homeLongitude);

    return true;
}

bool GamePlan::IsValid() const
{
    return _valid;
}

int GamePlan::GetVersion() const
{
    return _version;
}

double GamePlan::GetHomeLatitude() const
{
    return _homeLatitude;
}

double GamePlan::GetHomeLongitude() const
{
    return _homeLongitude;
}


const std::vector<Zone>& GamePlan::GetZones() const
{
    return _zones;
}