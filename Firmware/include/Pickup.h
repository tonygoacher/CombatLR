#pragma once

#include <stdint.h>
#include "GeoReference.h"

class Pickup
{
public:    
    double eastMetres;
    double northMetres;

    int type;

    uint32_t spawnTimeSeconds;
    uint32_t lifespanSeconds;

    int id;

    GameCoordinate GetPosition() const
    {
        GameCoordinate g;
        g.east = eastMetres;
        g.north = northMetres;
        return g;
    }
};