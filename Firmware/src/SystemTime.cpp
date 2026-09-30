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

    _startGpsTime = 0.0;
    _waitingForStart = false;
}

bool SystemTime::Reset(const char* startTime)
{
    double startGpsTime = 0.0;

    if (startTime == nullptr)
    {
        Serial.println("SystemTime::Reset start time is null");
        return false;
    }

    Serial.printf("SystemTime::Reset %s\n", startTime);

    if (strnlen(startTime, 9) != 8)
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
     * If GPS is already available and the start time has passed,
     * establish the clock immediately. This handles late players.
     */
    if (_navigation.HasPosition())
    {
        const double gpsTime = _navigation.GetGpsTime();

        if (gpsTime > 0.0)
        {
            const double delta =
                TimeDifference(gpsTime, _startGpsTime);

            if (delta >= 0.0)
            {
                _baseSystemMilliseconds =
                    static_cast<uint64_t>(delta * 1000.0);

                _baseMillis = millis();
                _baseGpsTime = gpsTime;
                _lastGpsTime = gpsTime;
                _gpsSynchronized = true;
                _waitingForStart = false;

                Serial.printf(
                    "SystemTime started at %lu seconds\n",
                    static_cast<unsigned long>(
                        _baseSystemMilliseconds / 1000));
            }
        }
    }

    return true;
}

void SystemTime::Update()
{
    const uint32_t nowMillis = millis();

    /*
     * No GPS: leave the base untouched. GetMilliseconds()
     * continues using millis(), so the clock keeps running.
     */
    if (!_navigation.HasPosition())
    {
        Serial.println("No position");
        return;
    }    
    const double gpsTime = _navigation.GetGpsTime();

    if (gpsTime <= 0.0)
        return;

    /*
     * Waiting for the absolute game start. Establish T=0
     * as soon as GPS reaches it.
     */
    if (_waitingForStart)
    {
        const double delta =
            TimeDifference(gpsTime, _startGpsTime);

        if (delta < 0.0)
            return;

        _baseSystemMilliseconds = 0;
        _baseMillis = nowMillis;
        _baseGpsTime = gpsTime;
        _lastGpsTime = gpsTime;
        _gpsSynchronized = true;
        _waitingForStart = false;

        return;
    }

    /*
     * Absolute game start mode. Re-anchor directly to the
     * original start time. This gives late players the same
     * game time as everyone else and avoids accumulated drift.
     */
    if (_startGpsTime != 0.0)
    {
        const double gameDelta =
            TimeDifference(gpsTime, _startGpsTime);

        if (gameDelta < 0.0)
            return;

        _baseSystemMilliseconds =
            static_cast<uint64_t>(gameDelta * 1000.0);

        _baseMillis = nowMillis;
        _baseGpsTime = gpsTime;
        _lastGpsTime = gpsTime;
        _gpsSynchronized = true;
        return;
    }

    /*
     * Local Reset() mode. GPS is used as an occasional
     * synchronisation reference while millis() provides
     * smooth progression between fixes.
     */
    if (!_gpsSynchronized)
    {
        const uint32_t elapsedMillis =
            nowMillis - _baseMillis;

        _baseSystemMilliseconds += elapsedMillis;

        _baseGpsTime = gpsTime;
        _lastGpsTime = gpsTime;
        _baseMillis = nowMillis;
        _gpsSynchronized = true;
        return;
    }

    const double gpsDelta =
        TimeDifference(gpsTime, _lastGpsTime);

    if (gpsDelta <= 0.0)
        return;

    const uint32_t elapsedMillis =
        nowMillis - _baseMillis;

    const double localSeconds =
        static_cast<double>(elapsedMillis) / 1000.0;

    if (fabs((gpsDelta - localSeconds) * 1000.0) >= 100.0)
    {
        _baseSystemMilliseconds +=
            static_cast<uint64_t>(gpsDelta * 1000.0);

        _baseMillis = nowMillis;
    }

    _lastGpsTime = gpsTime;
}

uint32_t SystemTime::GetMilliseconds() const
{
    if (_waitingForStart)
        return 0;

    const uint32_t elapsedMillis =
        millis() - _baseMillis;

    return static_cast<uint32_t>(
        _baseSystemMilliseconds + elapsedMillis);
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

double SystemTime::TimeDifference(
    double current,
    double reference)
{
    double delta = current - reference;

    if (delta < -43200.0)
        delta += 86400.0;
    else if (delta > 43200.0)
        delta -= 86400.0;

    return delta;
}
