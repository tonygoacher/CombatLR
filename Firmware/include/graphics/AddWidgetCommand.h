#pragma once

#include <string>
#include <utility>
#include "IDisplayCommand.h"
#include "widgets/Widget.h"
#include "GameCoordinate.h"
#include "PickupTypes.h"

class AddWidgetCommand final
    : public IDisplayCommand
{
public:
    AddWidgetCommand(
        uint32_t id,
        WidgetType widgetType,
        const GameCoordinate& position = { 0.0f, 0.0f },
        const std::string textureFilename = "",
        PickupTypes pickupType = PickupTypes::None)
        :
        IDisplayCommand(DisplayCommandType::AddWidget),
        _id(id),
        _widgetType(widgetType),
        _position(position),
        _textureFilename(textureFilename),
        _widget(nullptr),
        _pickupType(pickupType)
    {
        
    }

    Widget* PeekWidget() { return _widget == nullptr ? nullptr : _widget.get();}



    AddWidgetCommand(
        Widget* widget)
    :
        IDisplayCommand(DisplayCommandType::AddWidget),
        _widget(std::unique_ptr<Widget>( widget))
    {};

    uint32_t GetId() const
    {
        return _id;
    }

    WidgetType GetWidgetType() const
    {
        return _widgetType;
    }

    PickupTypes GetPickupType() const
    {
        return _pickupType;
    }

    const GameCoordinate& GetPosition() const
    {
        return _position;
    }

    const std::string& GetTextureFilename() const
    {
        return _textureFilename;
    }

    std::unique_ptr<Widget> GetWidget() { return std::move(_widget);}

private:
    uint32_t _id;
    WidgetType _widgetType;
    GameCoordinate _position;
    const std::string _textureFilename;
    std::unique_ptr<Widget> _widget;
    PickupTypes _pickupType;
};