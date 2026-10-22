#include "NavigationNMEA.h"
#include <math.h>

NavigationNMEA::NavigationNMEA()
{
    _rxPin = -1;
    _txPin = -1;

    _serial = new HardwareSerial(1);
}


void NavigationNMEA::Setup(int8_t rxPin,int8_t txPin, uint32_t baud)
{
     _baud = baud;
    _rxPin = rxPin;
    _txPin = txPin;
}


bool NavigationNMEA::Begin()
{
    if(_rxPin == -1 || _txPin == -1)
    {
        Serial.println("NavigationNMEA: Serial port not configured");
        return false;
    }

    _serial->begin(
        _baud,
        SERIAL_8N1,
        _rxPin,
        _txPin);

    _gps = TinyGPSPlus();

    _data = NavigationData{};

    _gpsTime = 0.0;

    _previousLatitude = 0.0;
    _previousLongitude = 0.0;
    _previousCourse = 0.0f;

    _previousPositionValid = false;
    _previousCourseValid = false;
    _havePreviousData = false;

    _filteredCourse = 0.0f;
    _filteredX = 0.0f;
    _filteredY = 0.0f;
    _haveFilteredCourse = false;

    return true;
}

bool NavigationNMEA::Update()
{
    while (_serial->available() > 0)
    {
        const char c =
            static_cast<char>(_serial->read());
        _gps.encode(c);
    }

    const bool positionValid =
        _gps.location.isValid();

    const bool courseValid =
        _gps.course.isValid();

    /*
     * Position
     */
    if (positionValid)
    {
        _data.latitude =
            _gps.location.lat();

        _data.longitude =
            _gps.location.lng();
    }

    /*
     * Altitude
     */
    if (_gps.altitude.isValid())
    {
        _data.altitude =
            static_cast<float>(
                _gps.altitude.meters());
    }

    /*
     * Speed
     */
    if (_gps.speed.isValid())
    {
        _data.speed =
            static_cast<float>(
                _gps.speed.mps());
    }

    /*
     * Raw GPS course
     */
    if (courseValid)
    {
        _data.course =
            static_cast<float>(
                _gps.course.deg());
    }

    /*
     * Filter GPS course into the game's heading.
     *
     * GPS course becomes unreliable at very low speed,
     * so only accept it once we're moving at walking speed.
     */
    const bool moving =
        _gps.speed.isValid() &&
        _gps.speed.mps() >= MIN_COURSE_SPEED;

    if (courseValid && moving)
    {
        const float course =
            static_cast<float>(
                _gps.course.deg());

        const float radians =
            course * DEG_TO_RAD;

        const float x = cosf(radians);
        const float y = sinf(radians);

        if (!_haveFilteredCourse)
        {
            /*
             * First valid course establishes the filter.
             */
            _filteredX = x;
            _filteredY = y;

            _haveFilteredCourse = true;
        }
        else
        {
            /*
             * Circular exponential filter.
             *
             * This avoids problems when crossing 0/360 degrees.
             */
            _filteredX =
                _filteredX * (1.0f - COURSE_FILTER_ALPHA) +
                x * COURSE_FILTER_ALPHA;

            _filteredY =
                _filteredY * (1.0f - COURSE_FILTER_ALPHA) +
                y * COURSE_FILTER_ALPHA;
        }

        float filtered =
            atan2f(_filteredY, _filteredX) *
            RAD_TO_DEG;

        if (filtered < 0.0f)
            filtered += 360.0f;

        _filteredCourse = filtered;

        /*
         * The application "heading" is now
         * the filtered GPS course.
         */
        _data.heading = _filteredCourse;
        _data.headingValid = true;
    }
    else
    {
        /*
         * Don't claim a valid heading while stationary.
         *
         * We deliberately retain the filtered value so that
         * when movement resumes the filter continues from
         * the previous direction.
         */
        _data.headingValid = false;
    }

    /*
     * Satellites
     */
    if (_gps.satellites.isValid())
    {
        const uint32_t satellites =
            _gps.satellites.value();

        _data.satellites =
            static_cast<uint8_t>(
                satellites > 255 ? 255 : satellites);
    }

    /*
     * Position validity
     */
    _data.positionValid = positionValid;

    /*
     * GPS time
     */
    if (_gps.time.isValid())
    {
        _gpsTime =
            static_cast<double>(_gps.time.hour()) * 3600.0 +
            static_cast<double>(_gps.time.minute()) * 60.0 +
            static_cast<double>(_gps.time.second()) +
            static_cast<double>(_gps.time.centisecond()) / 100.0;
    }

    /*
     * Detect changes in position/course.
     */
    bool changed = false;

    if (!_havePreviousData)
    {
        if (positionValid || courseValid)
        {
            _previousLatitude = _data.latitude;
            _previousLongitude = _data.longitude;
            _previousCourse = _data.course;

            _previousPositionValid = positionValid;
            _previousCourseValid = courseValid;

            _havePreviousData = true;

            changed = true;
        }
    }
    else
    {
        if (positionValid != _previousPositionValid)
        {
            changed = true;
        }
        else if (positionValid)
        {
            if (_data.latitude != _previousLatitude ||
                _data.longitude != _previousLongitude)
            {
                changed = true;
            }
        }

        if (courseValid != _previousCourseValid)
        {
            changed = true;
        }
        else if (courseValid)
        {
            if (_data.course != _previousCourse)
            {
                changed = true;
            }
        }

        if (changed)
        {
            _previousLatitude = _data.latitude;
            _previousLongitude = _data.longitude;
            _previousCourse = _data.course;

            _previousPositionValid = positionValid;
            _previousCourseValid = courseValid;
        }
    }

    return changed;
}

NavigationData NavigationNMEA::GetData() const
{

    return _data;
}

double NavigationNMEA::GetGpsTime() const
{
    return _gpsTime;
}



float NavigationNMEA::GetHDOP()  
{
    return _gps.hdop.hdop();
}
