#include "SystemTime.h"

#include <ctype.h>
#include <math.h>

SystemTime::SystemTime(INavigation& navigation)
    : _navigation(navigation)
{
}


void SystemTime::Reset()
{
    _baseMillis = millis();
    _baseSystemMilliseconds = 0;

    _gpsSynchronized = false;
    _baseGpsTime = 0.0;
    _lastGpsTime = 0.0;

    _waitingForStart = false;
    _startGpsTime = 0.0;
}


bool SystemTime::Reset(const char* startTime)
{
    double startGpsTime = 0.0;
    if(startTime == nullptr)
    {
        Serial.println("SystemTime::Reset start time is null");
        return false;
    }

    if(strnlen(startTime, MaxTimeLen+1) != MaxTimeLen)
    {
        Serial.println("SystemTime::Reset Invalid reset time");
        return false;
    }

    if (!ParseTime(startTime, startGpsTime))
    {
        Serial.printf("Invalid Reset time %s\n", startTime);
        return false;
    }        

    _baseMillis = millis();
    _baseSystemMilliseconds = 0;

    _gpsSynchronized = false;
    _baseGpsTime = 0.0;
    _lastGpsTime = 0.0;

    _startGpsTime = startGpsTime;
    _waitingForStart = true;

    /*
     * If GPS is already available, determine immediately
     * whether the requested start time has been reached.
     */
    if (_navigation.HasPosition())
    {
        const double gpsTime =
            _navigation.GetGpsTime();

        if (gpsTime > 0.0)
        {
            const double delta =
                TimeDifference(gpsTime, _startGpsTime);

            if (delta >= 0.0)
            {
                /*
                 * The game has already started. Establish
                 * T=delta immediately so a late player joins
                 * at the correct point in the game.
                 */
                _baseSystemMilliseconds =
                    static_cast<uint64_t>(delta * 1000.0);

                _baseMillis = millis();
                _baseGpsTime = gpsTime;
                _lastGpsTime = gpsTime;
                _gpsSynchronized = true;
                _waitingForStart = false;
            }
        }
    }

    return true;
}


void SystemTime::Update()
{
    const uint32_t nowMillis = millis();

    if (!_navigation.HasPosition())
        return;

    const double gpsTime =
        _navigation.GetGpsTime();

    if (gpsTime <= 0.0)
        return;

    /*
     * The configured start time is an absolute GPS wall-clock
     * time. Until GPS reaches it, game time remains exactly zero.
     */
    if (_waitingForStart)
    {
        const double delta =
            TimeDifference(gpsTime, _startGpsTime);

        if (delta < 0.0)
            return;

        /*
         * GPS has reached the start time. Establish T=0 here.
         * millis() then provides the smooth progression between
         * GPS updates.
         */
        _baseSystemMilliseconds = 0;
        _baseMillis = nowMillis;
        _baseGpsTime = gpsTime;
        _lastGpsTime = gpsTime;
        _gpsSynchronized = true;
        _waitingForStart = false;

        return;
    }

    /*
     * Once the game has started, GPS is the absolute calibration
     * reference. Calculate the true elapsed time directly from
     * the original start time, rather than accumulating GPS
     * deltas.
     *
     * This is important for late players: Reset() may be called
     * long after the original game start, and the same original
     * start time must produce the same game T on every device.
     */
    const double gameDelta =
        TimeDifference(gpsTime, _startGpsTime);

    if (gameDelta < 0.0)
        return;

    /*
     * Re-anchor the millis() clock at the GPS measurement.
     * GetMilliseconds() will add the millis() elapsed since
     * this point, giving smooth time between GPS updates.
     */
    _baseSystemMilliseconds =
        static_cast<uint64_t>(gameDelta * 1000.0);

    _baseMillis = nowMillis;
    _baseGpsTime = gpsTime;
    _lastGpsTime = gpsTime;
    _gpsSynchronized = true;
}


uint32_t SystemTime::GetMilliseconds()
{
    /*
     * Before the configured GPS start time, game time is zero.
     */
    if (_waitingForStart)
        return 0;

    const uint32_t elapsedMillis =
        millis() - _baseMillis;

    return static_cast<uint32_t>(
        _baseSystemMilliseconds +
        elapsedMillis);
}


uint32_t SystemTime::GetSeconds()
{
    return GetMilliseconds() / 1000;
}


bool SystemTime::ParseTime(
    const char* time,
    double& seconds)
{
    if (time == nullptr ||
        strlen(time) != 8 ||
        time[2] != ':' ||
        time[5] != ':')
    {
        return false;
    }

    if (!isdigit(time[0]) ||
        !isdigit(time[1]) ||
        !isdigit(time[3]) ||
        !isdigit(time[4]) ||
        !isdigit(time[6]) ||
        !isdigit(time[7]))
    {
        return false;
    }

    const int hours =
        (time[0] - '0') * 10 +
        (time[1] - '0');

    const int minutes =
        (time[3] - '0') * 10 +
        (time[4] - '0');

    const int secs =
        (time[6] - '0') * 10 +
        (time[7] - '0');

    if (hours > 23 ||
        minutes > 59 ||
        secs > 59)
    {
        return false;
    }

    seconds =
        static_cast<double>(
            hours * 3600 +
            minutes * 60 +
            secs);

    return true;
}


double SystemTime::TimeDifference(
    double current,
    double reference)
{
    double delta = current - reference;

    /*
     * GPS time is seconds since midnight. Choose the
     * shortest signed distance, allowing for midnight.
     */
    if (delta < -43200.0)
        delta += 86400.0;
    else if (delta > 43200.0)
        delta -= 86400.0;

    return delta;
}
