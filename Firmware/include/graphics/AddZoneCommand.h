
#pragma once

#include <string>
#include "IDisplayCommand.h"
#include "widgets/Widget.h"
#include "GameCoordinate.h"

class AddZoneCommand final
    : public IDisplayCommand
{
public:
    AddZoneCommand(
        uint32_t id,
        const GameCoordinate& position,
        const uint16_t radius,
        uint32_t colour)
        :
        IDisplayCommand(
            DisplayCommandType::AddZone),
        _id(id),
        _position(position),
        _radius(radius),
        _colour(colour)
    {
    }

    uint32_t GetId() const
    {
        return _id;
    }

    const GameCoordinate& GetPosition() const
    {
        return _position;
    }

    uint32_t GetColour() const
    {
        return _colour;
    }

    uint16_t GetRadius() const
    {
        return _radius;
    }

private:
    uint32_t _id;
    GameCoordinate _position;
    uint16_t _radius;
    uint32_t _colour;
};