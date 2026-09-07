#pragma once

#include "IDisplayCommand.h"

class DisplayBackgroundCommand final
    : public IDisplayCommand
{
public:
    explicit DisplayBackgroundCommand(
        const char* filename)
        :
        IDisplayCommand(
            DisplayCommandType::DisplayBackground),
        _filename(filename)
    {
    }

    const char* GetFilename() const
    {
        return _filename;
    }

private:
    const char* _filename;
};