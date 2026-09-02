#pragma once

#include "IDisplayCommand.h"

#include <stdint.h>

class EnableWidgetCommand final
    : public IDisplayCommand
{
public:
    explicit EnableWidgetCommand(uint32_t widgetId, bool enable)
        :
        IDisplayCommand(
            DisplayCommandType::EnableWidgetCommand),
            _widgetId(widgetId),
            _enable(enable)
    {
    }

    bool Enabled() const
    {
        return _enable;
    }

    uint32_t GetWidgetId() const
    {
        return _widgetId;
    }

private:
    bool    _enable;
    uint32_t _widgetId;
};