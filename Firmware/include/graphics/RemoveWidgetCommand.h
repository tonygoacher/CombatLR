#pragma once

#include "IDisplayCommand.h"

#include <stdint.h>

class RemoveWidgetCommand final
    : public IDisplayCommand
{
public:
    explicit RemoveWidgetCommand(uint32_t widgetId)
        :
        IDisplayCommand(
            DisplayCommandType::RemoveWidget),
        _widgetId(widgetId)
    {
    }

    uint32_t GetWidgetId() const
    {
        return _widgetId;
    }

private:
    uint32_t _widgetId;
};