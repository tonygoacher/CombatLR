#pragma once

enum class DisplayCommandType
{
    DisplayBackground,
    TurnMapToHeading,
    PlayerPosition,
    AddWidget,
    AddZone,
    RemoveWidget,
    Zoom,
    ShowPlayer,
    HidePlayer,
    ConfigureMap,
};

class IDisplayCommand
{
public:
    virtual ~IDisplayCommand() = default;

    DisplayCommandType GetType() const
    {
        return _type;
    }

protected:
    explicit IDisplayCommand(
        DisplayCommandType type)
        :
        _type(type)
    {
    }

private:
    DisplayCommandType _type;
};