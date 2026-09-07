#pragma once

#include "IDisplayCommand.h"
#include "GameCoordinate.h"
#include <stdint.h>

class SetZoneGeometryCommand final
    : public IDisplayCommand
{
public:
    explicit SetZoneGeometryCommand(uint32_t zoneId, GameCoordinate newPos, double radius)
       :
        IDisplayCommand(
            DisplayCommandType::SetZoneGeometry),
        _zoneId(zoneId),
        _gameCoordinate(newPos),
        _radius(radius)
    {
    }

    uint32_t GetId() const {return _zoneId;} 

    GameCoordinate GetCoordinate() const {return _gameCoordinate;} 

    double GetRadius() const { return _radius;}

private:
    GameCoordinate _gameCoordinate;
    uint32_t _zoneId;
    double _radius;
};