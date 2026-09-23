#include "MapReference.h"

#include <math.h>
#include <string.h>
#include <LittleFS.h>

MapReference::MapReference()
{
}

double MapReference::LatitudeScale(double latitudeDegrees)
{
    return cos(latitudeDegrees * DEG_TO_RAD_VALUE);
}

bool MapReference::Load(const char* filename)
{
    _loaded = false;
    _configured = false;

    for (int i = 0; i < POINT_COUNT; ++i)
    {
        _points[i] = ReferencePoint();
    }

    _cropX = 0.0;
    _cropY = 0.0;
    _cropWidth = 0.0;
    _cropHeight = 0.0;
    _metresPerPixel = 0.0;

    if (filename == nullptr)
        return false;

    File file = LittleFS.open(filename, "r");
    if (!file)
    {
        Serial.printf("MapReference: failed to open %s\n", filename);
        return false;
    }

    JsonDocument document;
    DeserializationError error = deserializeJson(document, file);
    file.close();

    if (error)
    {
        Serial.printf("MapReference: JSON parse failed: %s\n",
                      error.c_str());
        return false;
    }

    JsonObject sourceCrop = document["SourceCrop"].as<JsonObject>();
    if (sourceCrop.isNull())
    {
        Serial.println("MapReference: SourceCrop missing");
        return false;
    }

    _cropX = sourceCrop["X"].as<double>();
    _cropY = sourceCrop["Y"].as<double>();
    _cropWidth = sourceCrop["Width"].as<double>();
    _cropHeight = sourceCrop["Height"].as<double>();

    if (_cropWidth <= 0.0 || _cropHeight <= 0.0)
    {
        Serial.println("MapReference: invalid SourceCrop");
        return false;
    }

    JsonObject image = document["Image"].as<JsonObject>();
    if (!image.isNull())
    {
        int width = image["Width"].as<int>();
        int height = image["Height"].as<int>();

        if (width > 0)
            _imageWidth = width;

        if (height > 0)
            _imageHeight = height;
    }

    // The exported map is normally 1280x960. The JSON image dimensions
    // are retained so the reference also follows the file if that ever
    // changes.
    if (_imageWidth <= 0 || _imageHeight <= 0)
    {
        Serial.println("MapReference: invalid image dimensions");
        return false;
    }

    _metresPerPixel = document["MetresPerPixel"].as<double>();

    JsonObject reference = document["Reference"].as<JsonObject>();
    if (reference.isNull())
    {
        Serial.println("MapReference: Reference missing");
        return false;
    }

    JsonArray points = reference["Points"].as<JsonArray>();
    if (points.isNull())
    {
        Serial.println("MapReference: Reference.Points missing");
        return false;
    }

    for (JsonObject point : points)
    {
        const char* name = point["Name"] | "";

        int index = -1;

        if (strcmp(name, "A") == 0)
            index = 0;
        else if (strcmp(name, "B") == 0)
            index = 1;
        else if (strcmp(name, "C") == 0)
            index = 2;
        else if (strcmp(name, "D") == 0)
            index = 3;

        if (index < 0)
            continue;

        _points[index].sourceX = point["X"].as<double>();
        _points[index].sourceY = point["Y"].as<double>();

        JsonObject gps = point["Gps"].as<JsonObject>();
        if (gps.isNull())
        {
            Serial.printf("MapReference: GPS missing for point %s\n", name);
            return false;
        }

        _points[index].latitude = gps["Latitude"].as<double>();
        _points[index].longitude = gps["Longitude"].as<double>();
        _points[index].valid = true;
    }

    for (int i = 0; i < POINT_COUNT; ++i)
    {
        if (!_points[i].valid)
        {
            Serial.printf("MapReference: calibration point %c missing\n",
                          'A' + i);
            return false;
        }
    }

    if (!BuildImageToGeoTransform())
        return false;

    _loaded = true;

    Serial.println("MapReference loaded");
    Serial.printf("  Exported image: %dx%d\n",
                  _imageWidth, _imageHeight);

    Serial.printf("  Source crop: X=%.3f Y=%.3f W=%.3f H=%.3f\n",
                  _cropX, _cropY, _cropWidth, _cropHeight);

    for (int i = 0; i < POINT_COUNT; ++i)
    {
        Serial.printf("  %c source=(%.3f, %.3f) GPS=(%.8f, %.8f)\n",
                      'A' + i,
                      _points[i].sourceX,
                      _points[i].sourceY,
                      _points[i].latitude,
                      _points[i].longitude);
    }

    Serial.printf("  East transform:  %.12f*x + %.12f*y + %.12f\n",
                  _eastPerPixelX,
                  _eastPerPixelY,
                  _eastOffset);

    Serial.printf("  North transform: %.12f*x + %.12f*y + %.12f\n",
                  _northPerPixelX,
                  _northPerPixelY,
                  _northOffset);

    Serial.printf("  Metres/pixel: %.12f\n", _metresPerPixel);

    return true;
}

bool MapReference::Configure(double homeLatitude, double homeLongitude)
{
    if (!_loaded)
    {
        Serial.println("MapReference: Configure called before Load");
        return false;
    }

    // GameCoordinate is measured in metres east/north of the game home.
    // Work out where GPS point A lies in that coordinate system.
    //
    // This is deliberately the same local-earth approximation used by
    // MapBuilder.
    _homeLatitude = homeLatitude;
    _homeLongitude = homeLongitude;

    const double cosHomeLatitude = LatitudeScale(homeLatitude);

    _aEastFromHome =
        (_points[0].longitude - homeLongitude) *
        DEG_TO_RAD_VALUE *
        cosHomeLatitude *
        EARTH_RADIUS_METRES;

    _aNorthFromHome =
        (_points[0].latitude - homeLatitude) *
        DEG_TO_RAD_VALUE *
        EARTH_RADIUS_METRES;

    _configured = true;

    Serial.printf("MapReference configured: home=(%.8f, %.8f)\n",
                  homeLatitude,
                  homeLongitude);

    Serial.printf("  A relative to home: east=%.3f north=%.3f m\n",
                  _aEastFromHome,
                  _aNorthFromHome);

    return true;
}

bool MapReference::BuildImageToGeoTransform()
{
    // This is intentionally the same calculation as MapBuilder's
    // BuildImageToGeoTransform().
    //
    // GPS A is the local geographic origin.
    //
    // East:
    //   (longitude - longitudeA) * cos(latitudeA) * R
    //
    // North:
    //   (latitude - latitudeA) * R
    //
    // The four source-image calibration points are then used to solve
    // two independent affine least-squares equations.

    const double latitudeA = _points[0].latitude;
    const double longitudeA = _points[0].longitude;
    const double cosLatitudeA = LatitudeScale(latitudeA);

    double rows[POINT_COUNT][3];
    double east[POINT_COUNT];
    double north[POINT_COUNT];

    for (int i = 0; i < POINT_COUNT; ++i)
    {
        rows[i][0] = _points[i].sourceX;
        rows[i][1] = _points[i].sourceY;
        rows[i][2] = 1.0;

        east[i] =
            (_points[i].longitude - longitudeA) *
            DEG_TO_RAD_VALUE *
            cosLatitudeA *
            EARTH_RADIUS_METRES;

        north[i] =
            (_points[i].latitude - latitudeA) *
            DEG_TO_RAD_VALUE *
            EARTH_RADIUS_METRES;
    }

    double eastResult[3];
    double northResult[3];

    if (!SolveLeastSquares3(rows, east, eastResult))
    {
        Serial.println("MapReference: failed to solve east transform");
        return false;
    }

    if (!SolveLeastSquares3(rows, north, northResult))
    {
        Serial.println("MapReference: failed to solve north transform");
        return false;
    }

    _eastPerPixelX = eastResult[0];
    _eastPerPixelY = eastResult[1];
    _eastOffset = eastResult[2];

    _northPerPixelX = northResult[0];
    _northPerPixelY = northResult[1];
    _northOffset = northResult[2];

    return true;
}

bool MapReference::SolveLeastSquares3(
    const double rows[POINT_COUNT][3],
    const double values[POINT_COUNT],
    double result[3])
{
    // Form the normal equations:
    //
    //   (A^T A) x = A^T b
    //
    // This mirrors the solver used by MapBuilder.

    double matrix[3][4] = {};

    for (int row = 0; row < POINT_COUNT; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            for (int k = 0; k < 3; ++k)
            {
                matrix[col][k] += rows[row][col] * rows[row][k];
            }

            matrix[col][3] += rows[row][col] * values[row];
        }
    }

    // Gaussian elimination with partial pivoting.
    for (int column = 0; column < 3; ++column)
    {
        int pivot = column;
        double largest = fabs(matrix[column][column]);

        for (int row = column + 1; row < 3; ++row)
        {
            double value = fabs(matrix[row][column]);

            if (value > largest)
            {
                largest = value;
                pivot = row;
            }
        }

        if (largest < 1.0e-12)
            return false;

        if (pivot != column)
        {
            for (int col = column; col < 4; ++col)
            {
                double temp = matrix[column][col];
                matrix[column][col] = matrix[pivot][col];
                matrix[pivot][col] = temp;
            }
        }

        double divisor = matrix[column][column];

        for (int col = column; col < 4; ++col)
            matrix[column][col] /= divisor;

        for (int row = 0; row < 3; ++row)
        {
            if (row == column)
                continue;

            double factor = matrix[row][column];

            for (int col = column; col < 4; ++col)
                matrix[row][col] -= factor * matrix[column][col];
        }
    }

    result[0] = matrix[0][3];
    result[1] = matrix[1][3];
    result[2] = matrix[2][3];

    return true;
}

bool MapReference::GetSourcePixelFromGameCoordinate(
    const GameCoordinate& coordinate,
    double& sourceX,
    double& sourceY) const
{
    if (!_configured)
        return false;

    // Explicitly reconstruct the GPS position from the CombatLR
    // GameCoordinate. GameCoordinate is metres east/north of Home.
    const double latitude =
        _homeLatitude +
        ((double)coordinate.north /
         (EARTH_RADIUS_METRES * DEG_TO_RAD_VALUE));

    const double longitude =
        _homeLongitude +
        ((double)coordinate.east /
         (EARTH_RADIUS_METRES *
          DEG_TO_RAD_VALUE *
          LatitudeScale(_homeLatitude)));

    Serial.printf("  Game -> GPS: %.8f, %.8f\n",
                  latitude, longitude);

    // Now use exactly the same local geographic coordinate system as
    // MapBuilder: metres east/north relative to calibration point A.
    const double east =
        (longitude - _points[0].longitude) *
        DEG_TO_RAD_VALUE *
        LatitudeScale(_points[0].latitude) *
        EARTH_RADIUS_METRES;

    const double north =
        (latitude - _points[0].latitude) *
        DEG_TO_RAD_VALUE *
        EARTH_RADIUS_METRES;

    Serial.printf("  GPS -> local A: east=%.3f north=%.3f\n",
                  east, north);

    const double eastAdjusted = east - _eastOffset;
    const double northAdjusted = north - _northOffset;

    const double determinant =
        _eastPerPixelX * _northPerPixelY -
        _eastPerPixelY * _northPerPixelX;

    if (fabs(determinant) < 1.0e-12)
        return false;

    sourceX =
        (eastAdjusted * _northPerPixelY -
         _eastPerPixelY * northAdjusted) /
        determinant;

    sourceY =
        (_eastPerPixelX * northAdjusted -
         eastAdjusted * _northPerPixelX) /
        determinant;

    Serial.printf("  GPS -> Source: %.3f, %.3f\n",
                  sourceX, sourceY);

    return isfinite(sourceX) && isfinite(sourceY);
}

bool MapReference::GpsToMapPixel(
    double latitude,
    double longitude,
    float& mapX,
    float& mapY) const
{
    if (!_loaded)
        return false;

    const double east =
        (longitude - _points[0].longitude) *
        DEG_TO_RAD_VALUE *
        LatitudeScale(_points[0].latitude) *
        EARTH_RADIUS_METRES;

    const double north =
        (latitude - _points[0].latitude) *
        DEG_TO_RAD_VALUE *
        EARTH_RADIUS_METRES;

    const double eastAdjusted = east - _eastOffset;
    const double northAdjusted = north - _northOffset;

    const double determinant =
        _eastPerPixelX * _northPerPixelY -
        _eastPerPixelY * _northPerPixelX;

    if (fabs(determinant) < 1.0e-12)
        return false;

    const double sourceX =
        (eastAdjusted * _northPerPixelY -
         _eastPerPixelY * northAdjusted) /
        determinant;

    const double sourceY =
        (_eastPerPixelX * northAdjusted -
         eastAdjusted * _northPerPixelX) /
        determinant;

    mapX = (float)(
        (sourceX - _cropX) *
        ((double)_imageWidth / _cropWidth));

    mapY = (float)(
        (sourceY - _cropY) *
        ((double)_imageHeight / _cropHeight));

    return isfinite(mapX) && isfinite(mapY);
}

bool MapReference::GameToMapPixel(
    const GameCoordinate& coordinate,
    float& mapX,
    float& mapY) const
{
    if (!_configured)
        return false;

    const double latitude =
        _homeLatitude +
        ((double)coordinate.north /
         (EARTH_RADIUS_METRES * DEG_TO_RAD_VALUE));

    const double longitude =
        _homeLongitude +
        ((double)coordinate.east /
         (EARTH_RADIUS_METRES *
          DEG_TO_RAD_VALUE *
          LatitudeScale(_homeLatitude)));

    return GpsToMapPixel(latitude, longitude, mapX, mapY);
}

float MapReference::GetPixelsPerMetre() const
{
    if (_metresPerPixel <= 0.0)
        return 0.0f;

    return (float)(1.0 / _metresPerPixel);
}

float MapReference::GetMapUpHeading() const
{
    if (!_configured)
        return 0.0f;

    // The affine transform from GameCoordinate east/north to map X/Y
    // can be obtained by transforming the inverse image->geo matrix.
    //
    // For a unit north vector in the game coordinate system, determine
    // which direction it points in the map. This gives the heading of
    // map-up, expressed clockwise from true north.

    const double determinant =
        _eastPerPixelX * _northPerPixelY -
        _eastPerPixelY * _northPerPixelX;

    if (fabs(determinant) < 1.0e-12)
        return 0.0f;

    // Inverse image transform:
    //
    // sourceX = ixEast * east + ixNorth * north + constant
    // sourceY = iyEast * east + iyNorth * north + constant
    //
    // We only need the north column.
    const double sourceXPerNorth =
        -_eastPerPixelY / determinant;

    const double sourceYPerNorth =
        _eastPerPixelX / determinant;

    // Source pixel coordinates become exported map coordinates by
    // positive scale factors, so the direction is unchanged.
    //
    // Image Y increases downwards. Therefore the map-up vector is the
    // negative map-Y direction. Convert that map-up vector back into
    // east/north and calculate its true-north heading.
    const double mapUpEast = -sourceXPerNorth;
    const double mapUpNorth = -sourceYPerNorth;

    double heading =
        atan2(mapUpEast, mapUpNorth) / DEG_TO_RAD;

    while (heading < 0.0)
        heading += 360.0;

    while (heading >= 360.0)
        heading -= 360.0;

    return (float)heading;
}

bool MapReference::GetPointA(
    double& x, double& y,
    double& latitude, double& longitude) const
{
    if (!_points[0].valid)
        return false;

    x = _points[0].sourceX;
    y = _points[0].sourceY;
    latitude = _points[0].latitude;
    longitude = _points[0].longitude;
    return true;
}

bool MapReference::GetPointB(
    double& x, double& y,
    double& latitude, double& longitude) const
{
    if (!_points[1].valid)
        return false;

    x = _points[1].sourceX;
    y = _points[1].sourceY;
    latitude = _points[1].latitude;
    longitude = _points[1].longitude;
    return true;
}

bool MapReference::GetPointC(
    double& x, double& y,
    double& latitude, double& longitude) const
{
    if (!_points[2].valid)
        return false;

    x = _points[2].sourceX;
    y = _points[2].sourceY;
    latitude = _points[2].latitude;
    longitude = _points[2].longitude;
    return true;
}

bool MapReference::GetPointD(
    double& x, double& y,
    double& latitude, double& longitude) const
{
    if (!_points[3].valid)
        return false;

    x = _points[3].sourceX;
    y = _points[3].sourceY;
    latitude = _points[3].latitude;
    longitude = _points[3].longitude;
    return true;
}
