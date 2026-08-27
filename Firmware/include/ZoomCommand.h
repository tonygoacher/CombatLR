#pragma once

#include "IDisplayCommand.h"

class ZoomCommand final
    : public IDisplayCommand
{
public:
    explicit ZoomCommand(float zoom)
        :
        IDisplayCommand(
            DisplayCommandType::Zoom),
        _zoom(zoom)
    {
    }

    float GetZoom() const
    {
        return _zoom;
    }

private:
    float _zoom;
};