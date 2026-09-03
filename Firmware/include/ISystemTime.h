#pragma once


#include <stdint.h>


class ISystemTime
{
public:
    virtual void Reset() = 0;
    virtual uint32_t GetGameTime() = 0;
};