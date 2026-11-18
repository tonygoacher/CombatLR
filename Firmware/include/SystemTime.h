#pragma once

#include <Arduino.h>
#include "Navigation.h"
#include "ISystemTime.h"

class SystemTime : public ISystemTime
{
public:
    explicit SystemTime(INavigation& navigation);

    void Reset() override;
    bool Reset(const char* startTime) override;
    void Update() override;

    uint32_t GetSeconds();
    uint32_t GetMilliseconds() const;

private:
    enum class State
    {
        AwaitingReset,
        GPSOK,
        GPSLost,
        GPSCatchup
    };

    static bool ParseTime(const char* time, double& seconds);
    static double TimeDifference(double current, double reference);
    static double WrapGpsTime(double time);

    INavigation& _navigation;
    State _state = State::AwaitingReset;

    // The scheduled game start and the most recent usable GPS time.
    double _startGpsTime = 0.0;
    double _lastGpsTime = -1.0;
    uint32_t _lastGpsUpdateMillis = 0;

    // GPSCatchup holds the game time reached while GPS was unavailable.
    double _gpsCatchupTargetTime = 0.0;
    uint32_t _gpsCatchupMilliseconds = 0;
};
