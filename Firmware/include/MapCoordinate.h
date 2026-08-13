#pragma once

#include "GeoReference.h"

/*
 * Converts the game's Home-relative metre coordinate system into pixels
 * in the loaded map image.
 *
 * Map coordinates:
 *   X increases right/east
 *   Y increases down/south
 *
 * Game coordinates:
 *   east  increases right
 *   north increases up
 */
class MapCoordinate
{
public:
    MapCoordinate();

    /*
     * mapHomeX/mapHomeY are the pixel coordinates in the BMP corresponding
     * to Home.
     *
     * pixelsPerMetre is the map scale.
     */
    void Configure(
        float mapHomeX,
        float mapHomeY,
        float pixelsPerMetre);

    bool IsConfigured() const;

    float GetMapHomeX() const;
    float GetMapHomeY() const;
    float GetPixelsPerMetre() const;

    /*
     * Convert Home-relative game coordinates to BMP pixel coordinates.
     */
    void ToMapPixel(
        const GameCoordinate& coordinate,
        float& mapX,
        float& mapY) const;

private:
    float _mapHomeX;
    float _mapHomeY;
    float _pixelsPerMetre;
    bool _configured;
};
