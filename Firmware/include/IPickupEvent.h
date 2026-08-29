#pragma once

#include <stdint.h>

class IPickupEvent
{
public:
    virtual void PickupActivated(uint32_t pickupId) = 0;         // A timed pickup has been added to the map
    virtual void PickupDeactivated(uint32_t pickupId) = 0;       // A timed pickeup lifespan has expired.
};