#pragma once

#include <stdint.h>
#include "IRPowerLevel.h"
/*
 * MilesTag 2 shot packet.
 *
 * Wire format:
 *
 *     0 ppppppp tt dddd
 *
 *     ppppppp = player ID   (7 bits)
 *     tt      = team        (2 bits)
 *     dddd    = damage      (4 bits)
 *
 * Total = 14 bits.
 */
struct __attribute__((packed)) LaserTagData
{
    uint8_t playerId;
    uint8_t team;
    uint8_t shotPowerIndex;
};

class IRLaserTag
{
public:



    virtual void Init() = 0;

    virtual bool LaserTagDataAvailable() = 0;

    virtual bool GetLasertagData(
        LaserTagData& data) = 0;

    virtual void SendLasertagData(
        LaserTagData& dataToSend) = 0;

    virtual ~IRLaserTag() = default;

    virtual void SetIRPower(IRPowerLevel powerLevel) = 0;
};
