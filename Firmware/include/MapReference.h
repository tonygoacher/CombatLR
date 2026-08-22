#pragma once

#include <Arduino.h>
#include "GeoReference.h"

/*
 * MapReference
 *
 * Loads the map.json A/B calibration and converts GPS coordinates
 * into source-map pixel coordinates.
 *
 * The final map rotation is deliberately NOT taken from map.json.
 * A compass reading will be used later to establish the runtime
 * map rotation.
 */
class MapReference
{
public:
    bool Load(const char* filename);

    bool IsValid() const;

    int GetVersion() const;

    float GetImageWidth() const;
    float GetImageHeight() const;

    float GetMetresPerPixel() const;

    float GetPointAX() const;
    float GetPointAY() const;
    float GetPointBX() const;
    float GetPointBY() const;

    double GetGpsALatitude() const;
    double GetGpsALongitude() const;
    double GetGpsBLatitude() const;
    double GetGpsBLongitude() const;
    float GetMapUpHeading() const;

    /*
     * Convert a GPS position to source-map pixel coordinates.
     *
     * The returned coordinates are in the same coordinate system as
     * PointA/PointB from map.json.
     *
     * SourceCrop is not applied here. Keeping the calibration in the
     * original/source coordinate system makes the A/B transform explicit.
     */
bool GameToMapPixel(
    const GameCoordinate& position,
    float& x,
    float& y) const;

        bool Configure(
    double homeLatitude,
    double homeLongitude);

private:
    int _version = 0;

    float _imageWidth = 0.0f;
    float _imageHeight = 0.0f;

    float _metresPerPixel = 0.0f;

    float _pointAX = 0.0f;
    float _pointAY = 0.0f;
    float _pointBX = 0.0f;
    float _pointBY = 0.0f;

    double _gpsALatitude = 0.0;
    double _gpsALongitude = 0.0;
    double _gpsBLatitude = 0.0;
    double _gpsBLongitude = 0.0;
    float _mapUpHeading = 0.0f;

    /*
     * GameCoordinate -> map pixel transform.
     */
    double _eastToMapX = 0.0;
    double _eastToMapY = 0.0;
    double _northToMapX = 0.0;
    double _northToMapY = 0.0;

    /*
     * Map pixel corresponding to GameCoordinate (0,0),
     * i.e. GamePlan Home.
     */
    double _mapOriginX = 0.0;
    double _mapOriginY = 0.0;

    bool _valid = false;
    bool _configured = false;
};
