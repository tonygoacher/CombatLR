

#pragma once

#include "ISystemTime.h"
#include "Pacer.h"


class PacerTime : public ISystemTime
{
public:
    PacerTime();
    virtual void Reset()  override;
    virtual uint32_t GetGameTime() override;
private:
    Pacer _pacer;
    uint32_t _gameTime;    

};