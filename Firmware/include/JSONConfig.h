#pragma once

#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "IConfig.h"


class JSONConfig : public IConfig
{
public:

    explicit JSONConfig(
        fs::FS& filesystem,
        const char* filename = "/config.json");


    bool Load() override;


    const Config* GetConfig() const override;


private:

    fs::FS& _filesystem;

    const char* _filename;

    Config _config;


    bool ParseConfig(
        JsonDocument& document);


    bool ParsePlayer(
        JsonObjectConst object);


    bool ParseZones(
        JsonArrayConst array);


    bool ParsePickups(
        JsonArrayConst array);


    static bool ParseGPSPosition(
        JsonObjectConst object,
        GPSPosition& position);


    static bool ParseWeaponType(
        const char* name,
        WeaponType& type);


    static bool ParsePickupType(
        const char* name,
        PickupType& type);
};