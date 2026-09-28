#include "SystemTime.h"

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

    /*
     * If GPS is already available, use it immediately
     * as the reference.
     */
    if (_navigation.HasPosition())
    {
        const double gpsTime = _navigation.GetGpsTime();

        if (gpsTime > 0.0)
        {
            _baseGpsTime = gpsTime;
            _lastGpsTime = gpsTime;
            _gpsSynchronized = true;
        }
    }
}


void SystemTime::Update()
{
    const uint32_t nowMillis = millis();

    /*
     * Nothing to synchronise to if GPS isn't currently
     * providing a valid position/time.
     */
    if (!_navigation.HasPosition())
        return;

    const double gpsTime = _navigation.GetGpsTime();

    if (gpsTime <= 0.0)
        return;

    /*
     * First GPS fix after Reset(), or after a period where
     * GPS was unavailable.
     */
    if (!_gpsSynchronized)
    {
        /*
         * Our local clock has continued running using millis().
         */
        const uint32_t elapsedMillis =
            nowMillis - _baseMillis;

        _baseSystemMilliseconds += elapsedMillis;

        /*
         * GPS time now becomes the new reference.
         */
        _baseGpsTime = gpsTime;
        _lastGpsTime = gpsTime;
        _baseMillis = nowMillis;
        _gpsSynchronized = true;

        return;
    }

    /*
     * Work out how much GPS time has elapsed since the
     * previous synchronisation.
     *
     * GPS time is seconds since midnight, so handle the
     * midnight rollover.
     */
    double gpsDelta = gpsTime - _lastGpsTime;

    if (gpsDelta < -43200.0)
        gpsDelta += 86400.0;
    else if (gpsDelta > 43200.0)
        gpsDelta -= 86400.0;

    /*
     * Only synchronise when the GPS time has actually moved.
     */
    if (gpsDelta <= 0.0)
        return;

    /*
     * Calculate how far our local millis clock has moved.
     */
    const uint32_t elapsedMillis =
        nowMillis - _baseMillis;

    const double localSeconds =
        static_cast<double>(elapsedMillis) / 1000.0;

    /*
     * GPS gives us an absolute reference, while millis()
     * gives us the smooth local progression between fixes.
     *
     * Rather than jumping the clock to GPS time, establish
     * a new base using the GPS elapsed time.
     */
    if (fabs((gpsDelta - localSeconds) * 1000.0) >=
        GPS_RESYNC_THRESHOLD_MS)
    {
        /*
         * Move the accumulated clock forward by the GPS
         * measurement.
         */
        _baseSystemMilliseconds +=
            static_cast<uint64_t>(gpsDelta * 1000.0);

        _baseMillis = nowMillis;
    }

    _lastGpsTime = gpsTime;
}


uint32_t SystemTime::GetMilliseconds() const
{
    const uint32_t elapsedMillis =
        millis() - _baseMillis;

    return static_cast<uint32_t>(
        _baseSystemMilliseconds +
        elapsedMillis);
}


uint32_t SystemTime::GetSeconds() const
{
    return GetMilliseconds() / 1000;
}