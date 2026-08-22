#pragma once

#include <Arduino.h>
#include "Navigation.h"

class NavigationNMEA : public INavigation
{
public:
    NavigationNMEA(
        int8_t rxPin,
        int8_t txPin,
        uint32_t baud = 9600);

    bool Begin() override;
    bool Update() override;
    NavigationData GetData() const override;

private:
    static constexpr size_t LINE_BUFFER_SIZE = 128;

    HardwareSerial* _serial;
    int8_t _rxPin;
    int8_t _txPin;
    uint32_t _baud;

    char _line[LINE_BUFFER_SIZE];
    size_t _lineLength = 0;

    NavigationData _data{};

    bool ProcessSentence(const char* sentence);
    bool ValidateChecksum(const char* sentence) const;

    void ProcessGGA(const char* fields);
    void ProcessRMC(const char* fields);

    static double ParseCoordinate(
        const char* value,
        const char* hemisphere);

    static bool ParseFloat(
        const char* text,
        float& value);

    static bool ParseField(
        const char*& cursor,
        char* field,
        size_t fieldSize);
};
