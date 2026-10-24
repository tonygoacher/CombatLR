
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

    uint32_t GetSeconds() ;
    uint32_t GetMilliseconds() const;

private:
    static bool ParseTime(const char* time, double& seconds);
    static double TimeDifference(double current, double reference);

    INavigation& _navigation;

    uint32_t _baseMillis = 0;
    uint64_t _baseSystemMilliseconds = 0;

    double _baseGpsTime = 0.0;
    bool _gpsSynchronized = false;
    double _lastGpsTime = 0.0;
    uint32_t _lastGpsUpdateMillis = 0;
    bool _gpsFeedStale = false;

    double _gpsCatchupTargetTime = 0.0;
    uint64_t _frozenSystemMilliseconds = 0;
    bool _waitingForGpsCatchup = false;

    double _startGpsTime = 0.0;
    bool _waitingForStart = false;
};
