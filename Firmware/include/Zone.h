#pragma once

#include <map>
#include "Pickup.h"

struct Zone
{
    double eastMetres;
    double northMetres;
    double radiusMetres;

    uint32_t startTime;
    uint32_t lifespanSeconds;
    uint32_t id;

    std::map<int, Pickup> pickups;
};
