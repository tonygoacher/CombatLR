#pragma once

#include <Arduino.h>
#include "Navigation.h"
#include "ISystemTime.h"

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
 * Reset("HH:MM:SS") defines the GPS wall-clock time at which
 * game time T=0 occurs.  If the requested start time is still
 * in the future, the clock remains at zero until GPS reaches it.
 */
class SystemTime : public ISystemTime
{
public:

    explicit SystemTime(INavigation& navigation);

    /*
     * Reset the system clock to zero immediately.
     *
     * Retained for callers which simply want a local zero.
     */
    void Reset();

    /*
     * Reset the system clock using a GPS wall-clock start time.
     *
     * startTime must be in HH:MM:SS format.
     *
     * If the start time is in the future, GetSeconds() 
     * return zero until GPS reaches it.
     *
     * If the start time has already passed, the clock starts
     * at the corresponding elapsed time. This is important for
     * players joining an already-running game: they can use the
     * original game start time and immediately obtain T+n.
     */
    bool Reset(const char* startTime) override;

    /*
     * Update the clock.
     *
     * This should be called regularly from the game loop.
     * It allows the clock to re-synchronise to GPS.
     */
    void Update();

    /*
     * Return seconds elapsed since the configured game start.
     */
    uint32_t GetSeconds();



private:

    static bool ParseTime(
        const char* time,
        double& seconds);

    static double TimeDifference(
        double current,
        double reference);

    uint32_t GetMilliseconds();

    INavigation& _navigation;

    uint32_t _baseMillis = 0;

    /*
     * System time, in milliseconds, at _baseMillis.
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
     * When true, GPS has not yet reached the configured
     * game start time.
     */
    bool _waitingForStart = false;

    /*
     * Game start time, in seconds since midnight.
     */
    double _startGpsTime = 0.0;

    /*
     * Prevent a GPS update from causing the clock to jump
     * backwards because of GPS/NMEA timing jitter.
     */
    static constexpr uint32_t GPS_RESYNC_THRESHOLD_MS = 100;

    static const int MaxTimeLen = 8;//HH:MM:SS
};
