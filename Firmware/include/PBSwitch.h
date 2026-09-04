#ifndef PBSWITCH_H
#define PBSWITCH_H

#include <Arduino.h>

class PBSwitch {
public:
    // Defaults to oneShot mode = true
    PBSwitch(bool oneShot = true);
    PBSwitch(uint8_t port, bool oneShot = true);

    void Init(uint8_t port);
    
    // Returns true once per press if oneShot, or continuously if not
    bool Pressed(); 
    
    // Returns the true logical state (true = physically down)
    bool RawRead(); 

private:
    uint8_t _port;
    bool _oneShot;
    
    // State variables
    bool _debouncedState;
    bool _lastDebouncedState;
    
    // Debounce timer variables
    unsigned long _lastDebounceTime;
    const unsigned long _debounceDelay = 50; // 50ms debounce window
};

#endif