#pragma once

#include "IZoom.h"

#include "PBSwitch.h"

class SteppedZoom : public IZoom
{
public:
    SteppedZoom(int port);
    bool GetZoom(float & zoom) override;
     void Reset() override;

private:
    int _port;
    int _current;
    int _last;
    PBSwitch _switch;
};
