#pragma once


#include <stdint.h>


class ISystemTime
{
public:
    virtual void Reset() = 0;
    virtual bool Reset(const char* startTime) = 0;
    virtual void Update() = 0;

    /*
     * Return seconds elapsed since the configured game start.
     */
    virtual uint32_t GetSeconds() = 0;

    /*
     * Return elapsed time with millisecond resolution.
     */
};