#pragma once

#include <Arduino.h>

class PBSwitch {
public:
    // Constructor without port initialization
    PBSwitch(bool oneShot = false);

    // Constructor with port initialization
    PBSwitch(uint8_t port, bool oneShot = false);

    void Init(uint8_t port);

    // Read instantaneous physical pin state
    bool RawRead();

    // Read debounced state (does not consume one-shot transition)
    bool Read();

    // Read button press (one-shot or continuous)
    bool Pressed();

private:
    bool UpdateDebounce();

    uint8_t _port;
    bool _oneShot;

    bool _debouncedState;
    bool _lastDebouncedState;

    unsigned long _lastDebounceTime;
    unsigned long _debounceDelay = 50;
};