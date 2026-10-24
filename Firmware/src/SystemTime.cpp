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
    _lastGpsUpdateMillis = _baseMillis;

    _gpsCatchupTargetTime = 0.0;
    _frozenSystemMilliseconds = 0;
    _waitingForGpsCatchup = false;

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
    _lastGpsUpdateMillis = _baseMillis;

    _gpsCatchupTargetTime = 0.0;
    _frozenSystemMilliseconds = 0;
    _waitingForGpsCatchup = false;

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
                _lastGpsUpdateMillis = _baseMillis;
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
     * If GPS returns behind the frozen clock, do not let time run
     * backwards. Resume only when GPS reaches the time the local
     * clock had advanced to. TimeDifference handles midnight rollover.
     */
    if (_waitingForGpsCatchup)
    {
        if (TimeDifference(gpsTime, _gpsCatchupTargetTime) < 0.0)
            return;

        _baseSystemMilliseconds = _frozenSystemMilliseconds;
        _baseMillis = nowMillis;
        _baseGpsTime = gpsTime;
        _lastGpsTime = gpsTime;
        _lastGpsUpdateMillis = nowMillis;
        _gpsSynchronized = true;
        _waitingForGpsCatchup = false;
        return;
    }

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
        _lastGpsUpdateMillis = nowMillis;
        _gpsSynchronized = true;
        _waitingForStart = false;

        return;
    }

    /*
     * Absolute game start mode. Re-anchor to the original start time,
     * except when the restored GPS clock is over one second behind the
     * millis-based game clock. In that case freeze until GPS catches up.
     */
    if (_startGpsTime != 0.0)
    {
        const double gameDelta =
            TimeDifference(gpsTime, _startGpsTime);

        if (gameDelta < 0.0)
            return;

        const uint64_t gpsGameMilliseconds =
            static_cast<uint64_t>(gameDelta * 1000.0);
        const uint32_t localGameMilliseconds = GetMilliseconds();

        if (static_cast<uint64_t>(localGameMilliseconds) >
            gpsGameMilliseconds + 1000ULL)
        {
            _frozenSystemMilliseconds = localGameMilliseconds;
            _gpsCatchupTargetTime = _startGpsTime +
                static_cast<double>(localGameMilliseconds) / 1000.0;
            if (_gpsCatchupTargetTime >= 86400.0)
                _gpsCatchupTargetTime -= 86400.0;

            _waitingForGpsCatchup = true;

            Serial.printf(
                "SystemTime frozen for GPS catch-up; target %.3f\n",
                _gpsCatchupTargetTime);
            return;
        }

        _baseSystemMilliseconds = gpsGameMilliseconds;
        _baseMillis = nowMillis;
        _baseGpsTime = gpsTime;
        _lastGpsTime = gpsTime;
        _lastGpsUpdateMillis = nowMillis;
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
        _lastGpsUpdateMillis = nowMillis;
        _baseMillis = nowMillis;
        _gpsSynchronized = true;
        return;
    }

    const uint32_t sinceLastGpsMillis =
        nowMillis - _lastGpsUpdateMillis;
    const double gpsDelta =
        TimeDifference(gpsTime, _lastGpsTime);
    const double expectedGpsAdvance =
        static_cast<double>(sinceLastGpsMillis) / 1000.0;

    if (expectedGpsAdvance - gpsDelta > 1.0)
    {
        _frozenSystemMilliseconds = GetMilliseconds();
        _gpsCatchupTargetTime = _lastGpsTime + expectedGpsAdvance;
        if (_gpsCatchupTargetTime >= 86400.0)
            _gpsCatchupTargetTime -= 86400.0;

        _waitingForGpsCatchup = true;

        Serial.printf(
            "SystemTime frozen for GPS catch-up; target %.3f\n",
            _gpsCatchupTargetTime);
        return;
    }

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
    _lastGpsUpdateMillis = nowMillis;
}

uint32_t SystemTime::GetMilliseconds() const
{
    if (_waitingForStart)
        return 0;

    if (_waitingForGpsCatchup)
        return static_cast<uint32_t>(_frozenSystemMilliseconds);

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
