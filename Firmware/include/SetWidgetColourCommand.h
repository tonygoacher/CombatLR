#pragma once

#include "IDisplayCommand.h"

#include <stdint.h>

class SetWidgetColourCommand final
    : public IDisplayCommand
{
public:
    explicit SetWidgetColourCommand(uint32_t widgetId, uint32_t colour)
        :
        IDisplayCommand(
            DisplayCommandType::SetWidgetColour),
            _widgetId(widgetId),
            _colour(colour)
    {
    }

    uint32_t GetColour() const
    {
        return _colour;
    }

    uint32_t GetWidgetId() const
    {
        return _widgetId;
    }

private:
    uint32_t _colour;
    uint32_t _widgetId;
};