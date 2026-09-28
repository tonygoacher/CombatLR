#pragma once

#include <Arduino.h>
#include "Navigation.h"

/*
 * SystemTime
 *
 * Provides a monotonic game clock in seconds.
 *
 * GPS time is used as the authoritative reference whenever
 * a valid GPS position/time is available.
 *
 * millis() is used to continue the clock between GPS
 * synchronisations and when GPS is unavailable.
 *
 * Call Reset() to define time zero.
 */
class SystemTime
{
public:

    explicit SystemTime(INavigation& navigation);

    /*
     * Reset the system clock to zero.
     *
     * If GPS is currently available, GPS time becomes the
     * reference point.
     *
     * Otherwise millis() is used until GPS becomes available.
     */
    void Reset();

    /*
     * Update the clock.
     *
     * This should be called regularly from the game loop.
     * It allows the clock to re-synchronise to GPS.
     */
    void Update();

    /*
     * Return seconds elapsed since Reset().
     */
    uint32_t GetSeconds() const;

    /*
     * Return elapsed time with millisecond resolution.
     *
     * Useful internally and for anything that needs finer
     * resolution than whole seconds.
     */
    uint32_t GetMilliseconds() const;

private:

    INavigation& _navigation;

    uint32_t _baseMillis = 0;

    /*
     * System time, in milliseconds, at _baseMillis.
     *
     * This is the accumulated time since Reset().
     */
    uint64_t _baseSystemMilliseconds = 0;

    /*
     * GPS time at the last synchronisation, in seconds
     * since midnight.
     */
    double _baseGpsTime = 0.0;

    bool _gpsSynchronized = false;
    double _lastGpsTime = 0.0;

    /*
     * Prevent a GPS update from causing the clock to jump
     * backwards because of GPS/NMEA timing jitter.
     */
    static constexpr uint32_t GPS_RESYNC_THRESHOLD_MS = 100;
};