#pragma once

#include "IZoom.h"

#include "PBSwitch.h"

class SteppedZoom : public IZoom
{
public:
    SteppedZoom(PBSwitch& zoomSwitch);
    bool GetZoom(float & zoom) override;
    void Reset() override;

private:
    int _current;
    int _last;
    PBSwitch& _switch;
};
