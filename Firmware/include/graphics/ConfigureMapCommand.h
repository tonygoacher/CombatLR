#pragma once

#include "IDisplayCommand.h"

class ConfigureMapCommand final
    : public IDisplayCommand
{
public:
    struct MapConfigData
    {
        float mapUpHeading;
    };

    explicit ConfigureMapCommand(MapConfigData configData)
        :
        IDisplayCommand(
            DisplayCommandType::ConfigureMap),
        _mapConfigData(configData)
    {
    }

    const MapConfigData GetMapConfigData() const
    {
        return _mapConfigData;
    }

private:
    MapConfigData _mapConfigData;
};