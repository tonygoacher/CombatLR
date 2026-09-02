#pragma once

#include "IDisplayCommand.h"

#include <stdint.h>
#include <string>

class SetTextCommand final
    : public IDisplayCommand
{
public:
    explicit SetTextCommand(uint32_t widgetId, std::string newText)
        :
        IDisplayCommand(
            DisplayCommandType::SetTextCommand),
        _widgetId(widgetId),
        _newText(newText)
    {
    }

    uint32_t GetWidgetId() const
    {
        return _widgetId;
    }

    const std::string& GetText() const
    {
        return _newText;
    }


private:
    uint32_t _widgetId;
    const std::string _newText;
};