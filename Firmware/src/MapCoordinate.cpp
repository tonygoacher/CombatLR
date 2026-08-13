#include "MapCoordinate.h"

MapCoordinate::MapCoordinate()
    : _mapHomeX(0.0f),
      _mapHomeY(0.0f),
      _pixelsPerMetre(0.0f),
      _configured(false)
{
}

void MapCoordinate::Configure(
    float mapHomeX,
    float mapHomeY,
    float pixelsPerMetre)
{
    _mapHomeX = mapHomeX;
    _mapHomeY = mapHomeY;
    _pixelsPerMetre = pixelsPerMetre;

    _configured =
        pixelsPerMetre > 0.0f;
}

bool MapCoordinate::IsConfigured() const
{
    return _configured;
}

float MapCoordinate::GetMapHomeX() const
{
    return _mapHomeX;
}

float MapCoordinate::GetMapHomeY() const
{
    return _mapHomeY;
}

float MapCoordinate::GetPixelsPerMetre() const
{
    return _pixelsPerMetre;
}

void MapCoordinate::ToMapPixel(
    const GameCoordinate& coordinate,
    float& mapX,
    float& mapY) const
{
    if (!_configured)
    {
        mapX = 0.0f;
        mapY = 0.0f;
        return;
    }

    mapX =
        _mapHomeX +
        coordinate.east * _pixelsPerMetre;

    /*
     * Image Y increases downwards, while GameCoordinate::north increases
     * upwards.
     */
    mapY =
        _mapHomeY -
        coordinate.north * _pixelsPerMetre;
}
