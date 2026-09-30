#pragma once

#include "IDisplayCommand.h"

#include <string>



class SetWidgetTextureCommand : public IDisplayCommand
{
public:    
    SetWidgetTextureCommand(
        uint32_t widgetId,
        const char* filename)
        :
        IDisplayCommand(DisplayCommandType::SetWidgetTexture),
        _widgetId(widgetId),
        _filename(
            filename != nullptr
                ? filename
                : "")
    {
    }





    uint32_t GetWidgetId() const
    {
        return _widgetId;
    }


    const std::string&  GetTextureFilename() const
    {
        return _filename;
    }

private:

    uint32_t _widgetId;

    std::string _filename;

};