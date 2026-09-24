#pragma once

#include <Arduino.h>
#include <TinyGPSPlus.h>
#include "Navigation.h"
#include <string>

class NavigationNMEA : public INavigation
{
public:
    NavigationNMEA(
        int8_t rxPin,
        int8_t txPin,
        uint32_t baud = 9600);

    bool Begin() override;
    bool Update() override;
    float GetHDOP() override;

    NavigationData GetData() const override;
    double GetGpsTime() const override;



private:
    HardwareSerial* _serial;

    int8_t _rxPin;
    int8_t _txPin;
    uint32_t _baud;

    TinyGPSPlus _gps;

    NavigationData _data;

    double _gpsTime = 0.0;

    // Previous position/course used to detect changes
    double _previousLatitude = 0.0;
    double _previousLongitude = 0.0;
    float _previousCourse = 0.0f;

    bool _previousPositionValid = false;
    bool _previousCourseValid = false;
    bool _havePreviousData = false;

    // Filtered course
    float _filteredCourse = 0.0f;
    float _filteredX = 0.0f;
    float _filteredY = 0.0f;
    bool _haveFilteredCourse = false;

    // Course filtering
    static constexpr float MIN_COURSE_SPEED = 0.8f;  // m/s
    static constexpr float COURSE_FILTER_ALPHA = 0.30f;
};