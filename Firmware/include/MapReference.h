#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "GameCoordinate.h"

class MapReference
{
public:
    MapReference();

    // Load the map reference information from the map JSON file.
    bool Load(const char* filename);

    // Configure the reference for a game whose GameCoordinate origin is
    // the supplied GPS position.
    bool Configure(double homeLatitude, double homeLongitude);

    bool IsConfigured() const { return _configured; }

    // Convert a game coordinate (metres east/north of the game home)
    // into a pixel coordinate in the exported 1280x960 map.
    bool GameToMapPixel(const GameCoordinate& coordinate,
                        float& mapX,
                        float& mapY) const;

    // Direct GPS conversion using the same affine transform as MapBuilder.
    bool GpsToMapPixel(double latitude,
                       double longitude,
                       float& mapX,
                       float& mapY) const;

    // Heading of "up" in the exported map, in degrees clockwise from
    // true north.
    float GetMapUpHeading() const;

    // Pixels per metre in the exported map.
    float GetPixelsPerMetre() const;

    // Calibration point accessors. These are SOURCE IMAGE coordinates,
    // not coordinates in the exported 1280x960 image.
    bool GetPointA(double& x, double& y,
                   double& latitude, double& longitude) const;
    bool GetPointB(double& x, double& y,
                   double& latitude, double& longitude) const;
    bool GetPointC(double& x, double& y,
                   double& latitude, double& longitude) const;
    bool GetPointD(double& x, double& y,
                   double& latitude, double& longitude) const;

private:
    static constexpr int POINT_COUNT = 4;
    static constexpr double EARTH_RADIUS_METRES = 6371008.8;
    static constexpr double DEG_TO_RAD_VALUE = 0.01745329251994329576923690768489;

    struct ReferencePoint
    {
        double sourceX = 0.0;
        double sourceY = 0.0;
        double latitude = 0.0;
        double longitude = 0.0;
        bool valid = false;
    };

    ReferencePoint _points[POINT_COUNT];

    // Source-image -> local east/north affine transform.
    //
    // This deliberately mirrors MapBuilder:
    //
    //   east  = _eastPerPixelX  * x +
    //           _eastPerPixelY  * y + _eastOffset
    //
    //   north = _northPerPixelX * x +
    //           _northPerPixelY * y + _northOffset
    //
    // where x/y are ORIGINAL SOURCE IMAGE pixels.
    double _eastPerPixelX = 0.0;
    double _eastPerPixelY = 0.0;
    double _eastOffset = 0.0;

    double _northPerPixelX = 0.0;
    double _northPerPixelY = 0.0;
    double _northOffset = 0.0;

    // Source-image crop which becomes the exported 1280x960 map.
    double _cropX = 0.0;
    double _cropY = 0.0;
    double _cropWidth = 0.0;
    double _cropHeight = 0.0;

    int _imageWidth = 1280;
    int _imageHeight = 960;

    // MapBuilder's exported metres-per-pixel value.
    double _metresPerPixel = 0.0;

    // Local east/north coordinates of GPS point A relative to the
    // GameCoordinate origin.
    double _aEastFromHome = 0.0;
    double _aNorthFromHome = 0.0;

    double _homeLatitude = 0.0;
    double _homeLongitude = 0.0;

    bool _loaded = false;
    bool _configured = false;

    bool BuildImageToGeoTransform();

    static bool SolveLeastSquares3(const double rows[POINT_COUNT][3],
                                   const double values[POINT_COUNT],
                                   double result[3]);

    static double LatitudeScale(double latitudeDegrees);

    bool GetSourcePixelFromGameCoordinate(const GameCoordinate& coordinate,
                                          double& sourceX,
                                          double& sourceY) const;
};
