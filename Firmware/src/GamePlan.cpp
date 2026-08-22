#include "GamePlan.h"

#include <ArduinoJson.h>
#include <SPIFFS.h>

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
     * This initial implementation uses SPIFFS. If CombatLR is using
     * another filesystem for its map assets, this can be switched
     * without changing the GamePlan interface.
     */
    File file = SPIFFS.open(filename, FILE_READ);

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
