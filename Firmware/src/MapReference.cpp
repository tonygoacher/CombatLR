#include "MapReference.h"

#include <ArduinoJson.h>
#include <FS.h>
#include <math.h>
#include <SPIFFS.h>

#include "WorldConstants.h"

namespace
{
    constexpr double EARTH_RADIUS_METRES =
        6371000.0;

    /*
     * Convert two nearby GPS coordinates into a local east/north
     * displacement in metres.
     */
    void GpsDeltaMetres(
        double latitudeA,
        double longitudeA,
        double latitudeB,
        double longitudeB,
        double& east,
        double& north)
    {
        const double meanLatitude =
            (latitudeA + latitudeB) * 0.5 *
            DEG_TO_RAD;

        north =
            (latitudeB - latitudeA) *
            DEG_TO_RAD *
            EARTH_RADIUS_METRES;

        east =
            (longitudeB - longitudeA) *
            DEG_TO_RAD *
            EARTH_RADIUS_METRES *
            cos(meanLatitude);
    }
}

void printDouble( double val, unsigned int precision){
// prints val with number of decimal places determine by precision
// NOTE: precision is 1 followed by the number of zeros for the desired number of decimial places
// example: printDouble( 3.1415, 100); // prints 3.14 (two decimal places)

    Serial.print (int(val));  //prints the int part
    Serial.print("."); // print the decimal point
    unsigned int frac;
    if(val >= 0)
        frac = (val - int(val)) * precision;
    else
        frac = (int(val)- val ) * precision;

} 

bool MapReference::Load(const char* filename)
{
    _valid = false;

    if (filename == nullptr)
        return false;

    File file = SPIFFS.open(
        filename,
        FILE_READ);

    if (!file)
    {
        Serial.printf(
            "MapReference: failed to open %s\n",
            filename);

        return false;
    }

    JsonDocument document;

    const DeserializationError error =
        deserializeJson(
            document,
            file);

    file.close();

    if (error)
    {
        Serial.printf(
            "MapReference: JSON parse failed: %s\n",
            error.c_str());

        return false;
    }

    if (!document["Version"].is<int>() ||
        !document["Image"]["Width"].is<float>() ||
        !document["Image"]["Height"].is<float>())
    {
        Serial.println(
            "MapReference: missing image metadata");

        return false;
    }

    JsonObject reference =
        document["Reference"].as<JsonObject>();

    if (reference.isNull())
    {
        Serial.println(
            "MapReference: missing Reference");

        return false;
    }

    JsonObject pointA =
        reference["PointA"].as<JsonObject>();

    JsonObject pointB =
        reference["PointB"].as<JsonObject>();

    JsonObject gpsA =
        reference["GpsA"].as<JsonObject>();

    JsonObject gpsB =
        reference["GpsB"].as<JsonObject>();

    if (pointA.isNull() ||
        pointB.isNull() ||
        gpsA.isNull() ||
        gpsB.isNull())
    {
        Serial.println(
            "MapReference: incomplete A/B reference");

        return false;
    }

    if (!pointA["X"].is<float>() ||
        !pointA["Y"].is<float>() ||
        !pointB["X"].is<float>() ||
        !pointB["Y"].is<float>() ||
        !gpsA["Latitude"].is<double>() ||
        !gpsA["Longitude"].is<double>() ||
        !gpsB["Latitude"].is<double>() ||
        !gpsB["Longitude"].is<double>())
    {
        Serial.println(
            "MapReference: invalid A/B reference");

        return false;
    }

    _version =
        document["Version"].as<int>();

  _imageWidth =
    document["Image"]["Width"].as<float>();

_imageHeight =
    document["Image"]["Height"].as<float>();

_metresPerPixel =
    document["MetresPerPixel"].as<float>();

/*
 * Point A/B in map.json are expressed in the original
 * source-image coordinate system.
 *
 * map.bmp is the cropped image described by SourceCrop.
 * Convert the calibration points into map.bmp coordinates
 * here so the rest of MapReference operates entirely in
 * the loaded map coordinate system.
 */
JsonObject sourceCrop =
    document["SourceCrop"].as<JsonObject>();

if (sourceCrop.isNull() ||
    !sourceCrop["X"].is<float>() ||
    !sourceCrop["Y"].is<float>())
{
    Serial.println(
        "MapReference: missing SourceCrop");

    return false;
}

const float sourceCropX =
    sourceCrop["X"].as<float>();

const float sourceCropY =
    sourceCrop["Y"].as<float>();

_pointAX =
    pointA["X"].as<float>() -
    sourceCropX;

_pointAY =
    pointA["Y"].as<float>() -
    sourceCropY;

_pointBX =
    pointB["X"].as<float>() -
    sourceCropX;

_pointBY =
    pointB["Y"].as<float>() -
    sourceCropY;
    

    _gpsALatitude =
        gpsA["Latitude"].as<double>();

    _gpsALongitude =
        gpsA["Longitude"].as<double>();

    _gpsBLatitude =
        gpsB["Latitude"].as<double>();

    _gpsBLongitude =
        gpsB["Longitude"].as<double>();


    /*
     * Calculate the real-world A->B vector.
     */
    double gpsEast = 0.0;
    double gpsNorth = 0.0;

    GpsDeltaMetres(
        _gpsALatitude,
        _gpsALongitude,
        _gpsBLatitude,
        _gpsBLongitude,
        gpsEast,
        gpsNorth);

    const double gpsDistance =
        sqrt(
            gpsEast * gpsEast +
            gpsNorth * gpsNorth);

    /*
     * Calculate the source-map A->B vector.
     */
    const double mapX =
        static_cast<double>(_pointBX) -
        static_cast<double>(_pointAX);

    const double mapY =
        static_cast<double>(_pointBY) -
        static_cast<double>(_pointAY);

    const double mapDistance =
        sqrt(
            mapX * mapX +
            mapY * mapY);

    if (gpsDistance <= 0.0 ||
        mapDistance <= 0.0)
    {
        Serial.println(
            "MapReference: invalid A/B distance");

        return false;
    }

  

    _valid = true;

    Serial.printf(
        "MapReference: loaded version %d\n",
        _version);

    Serial.printf(
        "MapReference: image %.0fx%.0f\n",
        _imageWidth,
        _imageHeight);

    Serial.printf(
        "MapReference: A=(%.3f,%.3f) B=(%.3f,%.3f)\n",
        _pointAX,
        _pointAY,
        _pointBX,
        _pointBY);

    Serial.printf(
        "MapReference: GPS distance %.3fm, map distance %.3fpx\n",
        gpsDistance,
        mapDistance);

   
    return true;
}

bool MapReference::IsValid() const
{
    return _valid;
}

int MapReference::GetVersion() const
{
    return _version;
}

float MapReference::GetImageWidth() const
{
    return _imageWidth;
}

float MapReference::GetImageHeight() const
{
    return _imageHeight;
}

float MapReference::GetMetresPerPixel() const
{
    return _metresPerPixel;
}

float MapReference::GetPointAX() const
{
    return _pointAX;
}

float MapReference::GetPointAY() const
{
    return _pointAY;
}

float MapReference::GetPointBX() const
{
    return _pointBX;
}

float MapReference::GetPointBY() const
{
    return _pointBY;
}

double MapReference::GetGpsALatitude() const
{
    return _gpsALatitude;
}

double MapReference::GetGpsALongitude() const
{
    return _gpsALongitude;
}

double MapReference::GetGpsBLatitude() const
{
    return _gpsBLatitude;
}

double MapReference::GetGpsBLongitude() const
{
    return _gpsBLongitude;
}

float MapReference::GetPixelsPerMetre() const
{
    return static_cast<float>(
        sqrt(
            _eastToMapX * _eastToMapX +
            _eastToMapY * _eastToMapY));
}


bool MapReference::Configure(
    double homeLatitude,
    double homeLongitude)
{
    if (!_valid)
    {
        Serial.println(
            "MapReference: cannot configure before Load()");
        return false;
    }

    /*
     * Convert A and B from GPS into the local east/north
     * coordinate system whose origin is GamePlan Home.
     */
    double aEast = 0.0;
    double aNorth = 0.0;
    double bEast = 0.0;
    double bNorth = 0.0;

    GpsDeltaMetres(
        homeLatitude,
        homeLongitude,
        _gpsALatitude,
        _gpsALongitude,
        aEast,
        aNorth);

    GpsDeltaMetres(
        homeLatitude,
        homeLongitude,
        _gpsBLatitude,
        _gpsBLongitude,
        bEast,
        bNorth);

    /*
     * A -> B vector in GameCoordinate metres.
     */
    const double gameEast =
        bEast - aEast;

    const double gameNorth =
        bNorth - aNorth;

    /*
     * A -> B vector in map pixels.
     */
    const double mapX =
        static_cast<double>(_pointBX) -
        static_cast<double>(_pointAX);

    const double mapY =
        static_cast<double>(_pointBY) -
        static_cast<double>(_pointAY);

    const double gameLength =
        sqrt(
            gameEast * gameEast +
            gameNorth * gameNorth);

    const double mapLength =
        sqrt(
            mapX * mapX +
            mapY * mapY);

            /*
 * Calculate the compass bearing of the map's UP direction.
 *
 * BMP Y increases downwards, so map-up is the negative Y direction.
 */
const double mapAngleDegrees =
    atan2(
        mapY,
        mapX) *
    180.0 / M_PI;


    Serial.printf(
    "AB map vector: X=%.3f Y=%.3f\n",
    mapX,
    mapY);

const double gpsBearingDegrees =
    atan2(
        gameEast,
        gameNorth) *
    180.0 / M_PI;

_mapUpHeading =
    static_cast<float>(
        gpsBearingDegrees -
        mapAngleDegrees -
        90.0);



        Serial.printf(
    "Map angles: gps=%.3f map=%.3f\n",
    gpsBearingDegrees,
    mapAngleDegrees);


while (_mapUpHeading < 0.0f)
    _mapUpHeading += 360.0f;

while (_mapUpHeading >= 360.0f)
    _mapUpHeading -= 360.0f;

    if (gameLength <= 0.0 ||
        mapLength <= 0.0)
    {
        Serial.println(
            "MapReference: invalid A/B reference");

        return false;
    }

    /*
     * Unit vector along A -> B in GameCoordinate space.
     */
const double gameAlongEast =
    gameEast / gameLength;

const double gameAlongNorth =
    gameNorth / gameLength;

/*
 * Perpendicular GameCoordinate vector.
 *
 * This is the opposite handedness to the previous
 * implementation.  It matches the orientation used by
 * the mobile map transform.
 */
const double gameAcrossEast =
    gameAlongNorth;

const double gameAcrossNorth =
    -gameAlongEast;

    /*
     * Unit vector along A -> B in map space.
     */
    const double mapAlongX =
        mapX / mapLength;

    const double mapAlongY =
        mapY / mapLength;

    /*
     * Perpendicular map vector.
     */
    const double mapAcrossX =
        -mapAlongY;

    const double mapAcrossY =
        mapAlongX;

    const double pixelsPerMetre =
        mapLength / gameLength;

    /*
     * Build GameCoordinate -> map pixel transform.
     */
    _eastToMapX =
        pixelsPerMetre *
        (mapAlongX * gameAlongEast +
         mapAcrossX * gameAcrossEast);

    _eastToMapY =
        pixelsPerMetre *
        (mapAlongY * gameAlongEast +
         mapAcrossY * gameAcrossEast);

    _northToMapX =
        pixelsPerMetre *
        (mapAlongX * gameAlongNorth +
         mapAcrossX * gameAcrossNorth);

    _northToMapY =
        pixelsPerMetre *
        (mapAlongY * gameAlongNorth +
         mapAcrossY * gameAcrossNorth);

    /*
     * Find the BMP position of Home, which is
     * GameCoordinate (0,0).
     */
    _mapOriginX =
        static_cast<double>(_pointAX) -
        aEast * _eastToMapX -
        aNorth * _northToMapX;

    _mapOriginY =
        static_cast<double>(_pointAY) -
        aEast * _eastToMapY -
        aNorth * _northToMapY;

    _configured = true;

    Serial.printf(
    "Map origin diagnostic:\n"
    "  A game EN=(%.3f,%.3f)\n"
    "  origin=(%.3f,%.3f)\n"
    "  transform E=(%.6f,%.6f) N=(%.6f,%.6f)\n",
    aEast,
    aNorth,
    _mapOriginX,
    _mapOriginY,
    _eastToMapX,
    _eastToMapY,
    _northToMapX,
    _northToMapY);

    Serial.printf(
        "MapReference: Home map=(%.3f,%.3f)\n",
        _mapOriginX,
        _mapOriginY);

    Serial.printf(
        "MapReference: scale=%.6f pixels/metre\n",
        pixelsPerMetre);

        Serial.printf(
    "Map transform:\n"
    "  East  -> X=%+.4f Y=%+.4f\n"
    "  North -> X=%+.4f Y=%+.4f\n",
    _eastToMapX,
    _eastToMapY,
    _northToMapX,
    _northToMapY);

    Serial.printf(
    "MapReference: map-up heading=%.2f degrees\n",
    _mapUpHeading);

    return true;
}

bool MapReference::GameToMapPixel(
    const GameCoordinate& position,
    float& x,
    float& y) const
{
    if (!_valid || !_configured)
        return false;

    x = static_cast<float>(
        _mapOriginX +
        static_cast<double>(position.east) *
            _eastToMapX +
        static_cast<double>(position.north) *
            _northToMapX);

    y = static_cast<float>(
        _mapOriginY +
        static_cast<double>(position.east) *
            _eastToMapY +
        static_cast<double>(position.north) *
            _northToMapY);
           

    return true;
}

float MapReference::GetMapUpHeading() const
{
    Serial.printf("Map up in MapREf is %.3f\n", _mapUpHeading);
    return _mapUpHeading;
}