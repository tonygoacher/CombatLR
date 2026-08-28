#pragma once

#include <stdint.h>
#include "GeoReference.h"
#include "PickupTypes.h"

class Pickup
{
public:    
    double eastMetres;
    double northMetres;

    PickupTypes type;

    uint32_t spawnTimeSeconds;
    uint32_t lifespanSeconds;

    uint32_t id;

    GameCoordinate GetPosition() const
    {
        GameCoordinate g;
        g.east = eastMetres;
        g.north = northMetres;
        return g;
    }
};