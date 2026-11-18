#include "SystemTime.h"

#include <ctype.h>
#include <math.h>

namespace
{
    constexpr uint32_t GPS_STALE_TIMEOUT_MS = 5000;

    double WrapGpsTime(double time)
    {
        while (time >= 86400.0)
            time -= 86400.0;

        while (time < 0.0)
            time += 86400.0;

        return time;
    }
}

SystemTime::SystemTime(INavigation& navigation)
    : _navigation(navigation)
{
}

void SystemTime::Reset()
{
    _baseMillis = millis();
    _baseSystemMilliseconds = 0;

    _gpsSynchronized = false;
    _gpsTimeAcquired = false;
    _baseGpsTime = 0.0;
    _lastGpsTime = 0.0;
    _lastGpsUpdateMillis = _baseMillis;
    _gpsFeedStale = false;

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

    if (strnlen(startTime, 9) != 8 || !ParseTime(startTime, startGpsTime))
    {
        Serial.println("SystemTime::Reset Invalid reset time");
        return false;
    }

    const uint32_t nowMillis = millis();

    _baseMillis = nowMillis;
    _baseSystemMilliseconds = 0;

    _gpsSynchronized = false;
    _gpsTimeAcquired = false;
    _baseGpsTime = 0.0;
    _lastGpsTime = 0.0;
    _lastGpsUpdateMillis = nowMillis;
    _gpsFeedStale = false;

    _gpsCatchupTargetTime = 0.0;
    _frozenSystemMilliseconds = 0;
    _waitingForGpsCatchup = false;

    _startGpsTime = startGpsTime;
    _waitingForStart = true;

    // GPS time may be available before a valid position fix.
    const double gpsTime = _navigation.GetGpsTime();
    if (gpsTime > 0.0)
    {
        _gpsTimeAcquired = true;
        _lastGpsTime = gpsTime;
        _lastGpsUpdateMillis = nowMillis;

        const double delta = TimeDifference(gpsTime, _startGpsTime);
        if (delta >= 0.0)
        {
            _baseSystemMilliseconds =
                static_cast<uint64_t>(delta * 1000.0);
            _baseMillis = nowMillis;
            _baseGpsTime = gpsTime;
            _gpsSynchronized = true;
            _waitingForStart = false;

            Serial.printf(
                "SystemTime started at %lu seconds\n",
                static_cast<unsigned long>(_baseSystemMilliseconds / 1000));
        }
    }

    return true;
}

void SystemTime::Update()
{
    const uint32_t nowMillis = millis();
    const double gpsTime = _navigation.GetGpsTime();

    bool gpsAdvanced = false;

    /*
     * Track GPS clock acquisition and advancement independently of position.
     * TinyGPSPlus can retain a valid position after its time feed has stopped.
     */
    if (gpsTime > 0.0)
    {
        if (!_gpsTimeAcquired)
        {
            _gpsTimeAcquired = true;
            _lastGpsTime = gpsTime;
            _lastGpsUpdateMillis = nowMillis;
            gpsAdvanced = true;
        }
        else
        {
            const double gpsAdvance =
                TimeDifference(gpsTime, _lastGpsTime);

            if (gpsAdvance > 0.0)
            {
                gpsAdvanced = true;

                /*
                 * GPS has resumed after a stale period. Compare it with
                 * the time millis() extrapolated from the last GPS sample.
                 * If GPS is behind, hold the current game time until it
                 * catches up instead of moving the clock backwards.
                 */
                if (_gpsFeedStale)
                {
                    const double expectedGpsTime = WrapGpsTime(
                        _lastGpsTime +
                        static_cast<double>(
                            nowMillis - _lastGpsUpdateMillis) / 1000.0);

                    if (TimeDifference(gpsTime, expectedGpsTime) < 0.0)
                    {
                        _frozenSystemMilliseconds = GetMilliseconds();
                        _gpsCatchupTargetTime = expectedGpsTime;
                        _waitingForGpsCatchup = true;

                        Serial.printf(
                            "SystemTime: GPS returned behind local clock; "
                            "catch-up target %.3f\n",
                            _gpsCatchupTargetTime);
                    }
                }

                _lastGpsTime = gpsTime;
                _lastGpsUpdateMillis = nowMillis;
                _gpsFeedStale = false;
            }
        }
    }

    /*
     * A GPS clock is considered lost only after it has first been acquired
     * and then fails to advance for five seconds.
     */
    if (_gpsTimeAcquired && !gpsAdvanced &&
        nowMillis - _lastGpsUpdateMillis >= GPS_STALE_TIMEOUT_MS)
    {
        if (!_gpsFeedStale)
            Serial.println("SystemTime: GPS time stale; using millis()");

        _gpsFeedStale = true;
    }

    /*
     * While GPS is stale, GetMilliseconds() extrapolates from the last GPS
     * sample. Do not run the GPS start/catch-up logic against a frozen value.
     */
    if (_gpsFeedStale)
        return;

    if (!_gpsTimeAcquired || gpsTime <= 0.0)
        return;

    /*
     * GPS has returned but is behind the local extrapolated time.
     * Keep the game clock fixed until GPS reaches the saved target.
     */
    if (_waitingForGpsCatchup)
    {
        if (TimeDifference(gpsTime, _gpsCatchupTargetTime) < 0.0)
            return;

        _baseSystemMilliseconds = _frozenSystemMilliseconds;
        _baseMillis = nowMillis;
        _baseGpsTime = gpsTime;
        _gpsSynchronized = true;
        _waitingForGpsCatchup = false;

        Serial.println("SystemTime: GPS caught up");
    }

    /*
     * Wait for the scheduled absolute game start. This remains GPS-driven
     * until GPS has actually been acquired. If GPS later becomes stale,
     * GetMilliseconds() extrapolates the scheduled-start delta using millis.
     */
    if (_waitingForStart)
    {
        const double delta =
            TimeDifference(gpsTime, _startGpsTime);

        if (delta < 0.0)
            return;

        _baseSystemMilliseconds =
            static_cast<uint64_t>(delta * 1000.0);
        _baseMillis = nowMillis;
        _baseGpsTime = gpsTime;
        _gpsSynchronized = true;
        _waitingForStart = false;

        return;
    }

    /*
     * Absolute game start mode. GPS remains the reference when available.
     * If local time is ahead by more than one second, wait for GPS catch-up.
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
            _gpsCatchupTargetTime = WrapGpsTime(
                _startGpsTime +
                static_cast<double>(localGameMilliseconds) / 1000.0);
            _waitingForGpsCatchup = true;

            Serial.printf(
                "SystemTime frozen for GPS catch-up; target %.3f\n",
                _gpsCatchupTargetTime);
            return;
        }

        _baseSystemMilliseconds = gpsGameMilliseconds;
        _baseMillis = nowMillis;
        _baseGpsTime = gpsTime;
        _gpsSynchronized = true;
        return;
    }

    /*
     * Local Reset() mode. millis() starts the clock; the first GPS sample
     * establishes a synchronisation reference without losing elapsed time.
     */
    if (!_gpsSynchronized)
    {
        _baseSystemMilliseconds += nowMillis - _baseMillis;
        _baseMillis = nowMillis;
        _baseGpsTime = gpsTime;
        _gpsSynchronized = true;
        return;
    }

    /*
     * Local Reset() mode after synchronisation. Compare GPS elapsed time
     * with local elapsed time and retain the existing catch-up behaviour.
     */
    const double gpsDelta =
        TimeDifference(gpsTime, _baseGpsTime);
    const double localSeconds =
        static_cast<double>(nowMillis - _baseMillis) / 1000.0;

    if (localSeconds - gpsDelta > 1.0)
    {
        _frozenSystemMilliseconds = GetMilliseconds();
        _gpsCatchupTargetTime = WrapGpsTime(
            _baseGpsTime + localSeconds);
        _waitingForGpsCatchup = true;

        Serial.printf(
            "SystemTime frozen for GPS catch-up; target %.3f\n",
            _gpsCatchupTargetTime);
        return;
    }

    if (gpsDelta <= 0.0)
        return;

    if (fabs((gpsDelta - localSeconds) * 1000.0) >= 100.0)
    {
        _baseSystemMilliseconds +=
            static_cast<uint64_t>(gpsDelta * 1000.0);
        _baseMillis = nowMillis;
        _baseGpsTime = gpsTime;
    }
}

uint32_t SystemTime::GetMilliseconds() const
{
    if (_waitingForGpsCatchup)
        return static_cast<uint32_t>(_frozenSystemMilliseconds);

    if (_waitingForStart)
    {
        if (!_gpsTimeAcquired || !_gpsFeedStale)
            return 0;

        // Extrapolate absolute GPS time from the last acquired sample.
        const double elapsedSinceGps =
            static_cast<double>(
                millis() - _lastGpsUpdateMillis) / 1000.0;
        const double elapsedSinceStart =
            TimeDifference(_lastGpsTime, _startGpsTime) +
            elapsedSinceGps;

        if (elapsedSinceStart <= 0.0)
            return 0;

        return static_cast<uint32_t>(elapsedSinceStart * 1000.0);
    }

    return static_cast<uint32_t>(
        _baseSystemMilliseconds + (millis() - _baseMillis));
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
