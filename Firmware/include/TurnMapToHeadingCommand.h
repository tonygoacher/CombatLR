#pragma once

#include "IDisplayCommand.h"

class TurnMapToHeadingCommand final
    : public IDisplayCommand
{
public:
    explicit TurnMapToHeadingCommand(
        float heading)
        :
        IDisplayCommand(
            DisplayCommandType::TurnMapToHeading),
        _heading(heading)
    {
    }

    float GetHeading() const
    {
        return _heading;
    }

private:
    float _heading;
};