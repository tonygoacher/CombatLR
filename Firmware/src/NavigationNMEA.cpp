#include "NavigationNMEA.h"

#include <stdlib.h>
#include <string.h>

NavigationNMEA::NavigationNMEA(
    int8_t rxPin,
    int8_t txPin,
    uint32_t baud)
{

    _baud = baud;
    _rxPin = rxPin;
    _txPin = txPin;
    _serial = new HardwareSerial(1);
}

bool NavigationNMEA::Begin()
{
    _serial->begin(
        _baud,
        SERIAL_8N1,
        _rxPin,
        _txPin);

    _lineLength = 0;
    _data = NavigationData{};

    return true;
}

bool NavigationNMEA::Update()
{
    bool newPacket = false;

    /*
     * Non-blocking: only consume bytes already present in the UART
     * driver's receive buffer. No waiting for a character occurs here.
     */
    while (_serial->available() > 0)
    {
        const char c =
            static_cast<char>(_serial->read());

        if (c == '\n')
        {
            _line[_lineLength] = '\0';

            if (_lineLength != 0)
            {
                if (ProcessSentence(_line))
                {
                    newPacket = true;
                }                    
            }

            _lineLength = 0;
            continue;
        }

        if (c == '\r')
            continue;

        if (c == '$')
        {
            _lineLength = 0;
            _line[_lineLength++] = c;
            continue;
        }

        if (_lineLength == 0)
            continue;

        if (_lineLength >= LINE_BUFFER_SIZE - 1)
        {
            _lineLength = 0;
            continue;
        }

        _line[_lineLength++] = c;
    }

    return newPacket;
}

NavigationData NavigationNMEA::GetData() const
{
    return _data;
}

bool NavigationNMEA::ValidateChecksum(
    const char* sentence) const
{
    if (sentence == nullptr || sentence[0] != '$')
        return false;

    const char* star = strchr(sentence, '*');

    if (star == nullptr ||
        star[1] == '\0' ||
        star[2] == '\0')
        return false;

    uint8_t checksum = 0;

    for (const char* p = sentence + 1;
         p < star;
         ++p)
    {
        checksum ^=
            static_cast<uint8_t>(*p);
    }

    char* end = nullptr;
    const unsigned long supplied =
        strtoul(star + 1, &end, 16);

    if (end == star + 1 || supplied > 0xFF)
        return false;

    return checksum ==
        static_cast<uint8_t>(supplied);
}

bool NavigationNMEA::ProcessSentence(
    const char* sentence)
{
    if (!ValidateChecksum(sentence))
        return false;

    const char* comma =
        strchr(sentence, ',');

    if (comma == nullptr)
        return false;

    const size_t identifierLength =
        static_cast<size_t>(comma - (sentence + 1));

    if (identifierLength < 3)
        return false;

    const char* type = comma - 3;

    if (strncmp(type, "GGA", 3) == 0)
    {
        ProcessGGA(comma + 1);
        return true;
    }

    if (strncmp(type, "RMC", 3) == 0)
    {
        ProcessRMC(comma + 1);
        return true;
    }

    return false;
}

void NavigationNMEA::ProcessGGA(
    const char* fields)
{
    /*
     * GGA:
     * 0 time, 1 lat, 2 N/S, 3 lon, 4 E/W, 5 fix quality
     */
    char field[24];
    char latitude[20] = {};
    char latitudeHemisphere[4] = {};
    char longitude[20] = {};
    char longitudeHemisphere[4] = {};
    char fixQuality[8] = {};

    const char* cursor = fields;

    for (int index = 0; index <= 5; ++index)
    {
        if (!ParseField(cursor, field, sizeof(field)))
            return;

        switch (index)
        {
        case 1:
            strncpy(latitude, field, sizeof(latitude) - 1);
            break;

        case 2:
            strncpy(latitudeHemisphere, field,
                    sizeof(latitudeHemisphere) - 1);
            break;

        case 3:
            strncpy(longitude, field, sizeof(longitude) - 1);
            break;

        case 4:
            strncpy(longitudeHemisphere, field,
                    sizeof(longitudeHemisphere) - 1);
            break;

        case 5:
            strncpy(fixQuality, field, sizeof(fixQuality) - 1);
            break;

        default:
            break;
        }
    }

    if (atoi(fixQuality) <= 0)
    {
        _data.positionValid = false;
        return;
    }

    _data.latitude =
        ParseCoordinate(latitude, latitudeHemisphere);

    _data.longitude =
        ParseCoordinate(longitude, longitudeHemisphere);

    _data.positionValid = true;
}

void NavigationNMEA::ProcessRMC(
    const char* fields)
{
    /*
     * RMC:
     * 0 time, 1 status, 2 lat, 3 N/S, 4 lon, 5 E/W,
     * 6 speed, 7 course over ground
     */
    char field[24];
    char status[4] = {};
    char latitude[20] = {};
    char latitudeHemisphere[4] = {};
    char longitude[20] = {};
    char longitudeHemisphere[4] = {};
    char course[20] = {};

    const char* cursor = fields;

    for (int index = 0; index <= 7; ++index)
    {
        if (!ParseField(cursor, field, sizeof(field)))
            return;

        switch (index)
        {
        case 1:
            strncpy(status, field, sizeof(status) - 1);
            break;

        case 2:
            strncpy(latitude, field, sizeof(latitude) - 1);
            break;

        case 3:
            strncpy(latitudeHemisphere, field,
                    sizeof(latitudeHemisphere) - 1);
            break;

        case 4:
            strncpy(longitude, field, sizeof(longitude) - 1);
            break;

        case 5:
            strncpy(longitudeHemisphere, field,
                    sizeof(longitudeHemisphere) - 1);
            break;

        case 7:
            strncpy(course, field, sizeof(course) - 1);
            break;

        default:
            break;
        }
    }

    if (status[0] != 'A')
    {
        _data.positionValid = false;
        _data.headingValid = false;
        return;
    }

    _data.latitude =
        ParseCoordinate(latitude, latitudeHemisphere);

    _data.longitude =
        ParseCoordinate(longitude, longitudeHemisphere);

    _data.positionValid = true;

    float heading = 0.0f;

    if (ParseFloat(course, heading))
    {
        /*
         * This is NMEA course-over-ground, not magnetometer heading.
         * It is useful as a temporary heading source while the actual
         * magnetometer/UBX heading is implemented.
         */
        _data.heading = heading;
        _data.headingValid = true;
    }
}

double NavigationNMEA::ParseCoordinate(
    const char* value,
    const char* hemisphere)
{
    if (value == nullptr ||
        hemisphere == nullptr ||
        value[0] == '\0' ||
        hemisphere[0] == '\0')
        return 0.0;

    char* end = nullptr;
    const double nmea = strtod(value, &end);

    if (end == value)
        return 0.0;

    const int degrees =
        static_cast<int>(nmea / 100.0);

    const double minutes =
        nmea -
        static_cast<double>(degrees) * 100.0;

    double decimal =
        static_cast<double>(degrees) +
        minutes / 60.0;

    if (hemisphere[0] == 'S' ||
        hemisphere[0] == 'W')
        decimal = -decimal;

    return decimal;
}

bool NavigationNMEA::ParseFloat(
    const char* text,
    float& value)
{
    if (text == nullptr || text[0] == '\0')
        return false;

    char* end = nullptr;
    const double parsed = strtod(text, &end);

    if (end == text)
        return false;

    value = static_cast<float>(parsed);
    return true;
}

bool NavigationNMEA::ParseField(
    const char*& cursor,
    char* field,
    size_t fieldSize)
{
    if (cursor == nullptr ||
        field == nullptr ||
        fieldSize == 0)
        return false;

    size_t length = 0;

    while (*cursor != '\0' && *cursor != ',')
    {
        if (length < fieldSize - 1)
            field[length++] = *cursor;

        ++cursor;
    }

    field[length] = '\0';

    if (*cursor == ',')
        ++cursor;

    return true;
}
