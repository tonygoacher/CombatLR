#pragma once

#include <stdint.h>

struct Pickup
{
    double eastMetres;
    double northMetres;

    int type;

    uint32_t spawnTimeSeconds;
    uint32_t lifespanSeconds;

    int id;
};