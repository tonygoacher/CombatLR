#pragma once

#include "IDisplayCommand.h"
#include "GameCoordinate.h"

class PlayerPositionCommand final
    : public IDisplayCommand
{
public:
    explicit PlayerPositionCommand(
        const GameCoordinate& position)
        :
        IDisplayCommand(
            DisplayCommandType::PlayerPosition),
        _position(position)
    {
    }

    const GameCoordinate& GetPosition() const
    {
        return _position;
    }

private:
    GameCoordinate _position;
};