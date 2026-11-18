#include "SystemTime.h"

#include <ctype.h>
#include <math.h>

namespace
{
    constexpr uint32_t GPS_STALE_TIMEOUT_MS = 5000;
}

SystemTime::SystemTime(INavigation& navigation)
    : _navigation(navigation)
{
}

void SystemTime::Reset()
{
    _state = State::AwaitingReset;
    _startGpsTime = 0.0;
    _lastGpsTime = -1.0;
    _lastGpsUpdateMillis = millis();
    _gpsCatchupTargetTime = 0.0;
    _gpsCatchupMilliseconds = 0;
}

bool SystemTime::Reset(const char* startTime)
{
    double parsedStartTime = 0.0;

    if (startTime == nullptr ||
        strnlen(startTime, 9) != 8 ||
        !ParseTime(startTime, parsedStartTime))
    {
        Serial.println("SystemTime::Reset Invalid reset time");
        return false;
    }

    _startGpsTime = parsedStartTime;
    _gpsCatchupTargetTime = 0.0;
    _gpsCatchupMilliseconds = 0;

    const uint32_t nowMillis = millis();
    const double gpsTime = _navigation.GetGpsTime();

    if (gpsTime > 0.0)
    {
        _lastGpsTime = gpsTime;
        _lastGpsUpdateMillis = nowMillis;
        _state = State::GPSOK;
    }
    else
    {
        // No time reference exists yet. GPSLost waits for the first fix.
        _lastGpsTime = -1.0;
        _lastGpsUpdateMillis = nowMillis;
        _state = State::GPSLost;
    }

    Serial.printf("SystemTime::Reset %s, state=%s\n",
        startTime,
        _state == State::GPSOK ? "GPSOK" : "GPSLost");

    return true;
}

void SystemTime::Update()
{
    const uint32_t nowMillis = millis();
    const double gpsTime = _navigation.GetGpsTime();

    switch (_state)
    {
    case State::AwaitingReset:
        // Reset(startTime) is the only operation that starts the clock.
        return;

    case State::GPSOK:
    {
        if (gpsTime > 0.0)
        {
            const double gpsAdvance =
                TimeDifference(gpsTime, _lastGpsTime);

            if (gpsAdvance > 0.0)
            {
                _lastGpsTime = gpsTime;
                _lastGpsUpdateMillis = nowMillis;
                return;
            }
        }

        if (nowMillis - _lastGpsUpdateMillis >= GPS_STALE_TIMEOUT_MS)
        {
            _state = State::GPSLost;
            Serial.println("SystemTime: GPS time lost; using millis()");
        }
        return;
    }

    case State::GPSLost:
    {
        // If Reset happened without GPS, the first valid sample establishes
        // the reference. It does not imply that the scheduled start has passed.
        if (_lastGpsTime < 0.0)
        {
            if (gpsTime > 0.0)
            {
                _lastGpsTime = gpsTime;
                _lastGpsUpdateMillis = nowMillis;
                _state = State::GPSOK;
                Serial.println("SystemTime: GPS time acquired");
            }
            return;
        }

        if (gpsTime <= 0.0 ||
            TimeDifference(gpsTime, _lastGpsTime) <= 0.0)
        {
            return;
        }

        // GPS has resumed. Hold the exact game time reached by millis()
        // while lost, and wait for GPS to reach the corresponding time.
        const uint32_t elapsedMillis =
            nowMillis - _lastGpsUpdateMillis;

        _gpsCatchupTargetTime = WrapGpsTime(
            _lastGpsTime +
            static_cast<double>(elapsedMillis) / 1000.0);

        _gpsCatchupMilliseconds = GetMilliseconds();
        _state = State::GPSCatchup;

        Serial.printf(
            "SystemTime: GPS returned; holding at %lu ms until %.3f\n",
            static_cast<unsigned long>(_gpsCatchupMilliseconds),
            _gpsCatchupTargetTime);
        return;
    }

    case State::GPSCatchup:
    {
        if (gpsTime <= 0.0 ||
            TimeDifference(gpsTime, _gpsCatchupTargetTime) < 0.0)
        {
            return;
        }

        // GPS has caught up. Re-anchor it at the held game time.
        _lastGpsTime = gpsTime;
        _lastGpsUpdateMillis = nowMillis;
        _state = State::GPSOK;

        Serial.println("SystemTime: GPS caught up; GPS time restored");
        return;
    }
    }
}

uint32_t SystemTime::GetMilliseconds() const
{
    switch (_state)
    {
    case State::AwaitingReset:
        return 0;

    case State::GPSOK:
    case State::GPSLost:
    {
        if (_lastGpsTime < 0.0)
            return 0;

        const double elapsedSinceGps =
            static_cast<double>(millis() - _lastGpsUpdateMillis) / 1000.0;
        const double elapsedSinceStart =
            TimeDifference(_lastGpsTime, _startGpsTime) +
            elapsedSinceGps;

        if (elapsedSinceStart <= 0.0)
            return 0;

        return static_cast<uint32_t>(elapsedSinceStart * 1000.0);
    }

    case State::GPSCatchup:
        return _gpsCatchupMilliseconds;
    }

    return 0;
}

uint32_t SystemTime::GetSeconds()
{
    return GetMilliseconds() / 1000;
}

bool SystemTime::ParseTime(const char* time, double& seconds)
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
        (time[0] - '0') * 10 + (time[1] - '0');
    const int minutes =
        (time[3] - '0') * 10 + (time[4] - '0');
    const int secs =
        (time[6] - '0') * 10 + (time[7] - '0');

    if (hours > 23 || minutes > 59 || secs > 59)
        return false;

    seconds = static_cast<double>(
        hours * 3600 + minutes * 60 + secs);

    return true;
}

double SystemTime::TimeDifference(double current, double reference)
{
    double delta = current - reference;

    if (delta < -43200.0)
        delta += 86400.0;
    else if (delta > 43200.0)
        delta -= 86400.0;

    return delta;
}

double SystemTime::WrapGpsTime(double time)
{
    while (time >= 86400.0)
        time -= 86400.0;

    while (time < 0.0)
        time += 86400.0;

    return time;
}
