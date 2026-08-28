#pragma once

#include <string>
#include "IDisplayCommand.h"
#include "IWidget.h"
#include "GameCoordinate.h"

class AddWidgetCommand final
    : public IDisplayCommand
{
public:
    AddWidgetCommand(
        uint32_t id,
        WidgetType widgetType,
        const GameCoordinate& position,
        const std::string textureFilename)
        :
        IDisplayCommand(
            DisplayCommandType::AddWidget),
        _id(id),
        _widgetType(widgetType),
        _position(position),
        _textureFilename(textureFilename)
    {
    }

    uint32_t GetId() const
    {
        return _id;
    }

    WidgetType GetWidgetType() const
    {
        return _widgetType;
    }

    const GameCoordinate& GetPosition() const
    {
        return _position;
    }

    const std::string& GetTextureFilename() const
    {
        return _textureFilename;
    }

private:
    uint32_t _id;
    WidgetType _widgetType;
    GameCoordinate _position;
    const std::string _textureFilename;
};