#include "PBSwitch.h"

// Constructor without port initialization
PBSwitch::PBSwitch(bool oneShot) {
    _oneShot = oneShot;
    _port = 255; // Indicates uninitialized

    _debouncedState = false;
    _lastDebouncedState = false;
    _lastDebounceTime = 0;
}

// Constructor with port initialization
PBSwitch::PBSwitch(uint8_t port, bool oneShot) : PBSwitch(oneShot) {
    Init(port);
}

void PBSwitch::Init(uint8_t port) {
    _port = port;
    pinMode(_port, INPUT_PULLUP);

    // Initialize state based on current pin status
    _debouncedState = (digitalRead(_port) == LOW);
    _lastDebouncedState = _debouncedState;

    _lastDebounceTime = millis();
}

// Read instantaneous physical pin state
bool PBSwitch::RawRead() {
    if (_port == 255) return false;

    // INPUT_PULLUP: LOW means pressed
    return (digitalRead(_port) == LOW);
}

// Update and return debounced physical state
bool PBSwitch::UpdateDebounce() {
    if (_port == 255) return false;

    bool currentRawPressed = (digitalRead(_port) == LOW);

    if (currentRawPressed) {
        // IMMEDIATE ON
        _lastDebounceTime = millis();
        _debouncedState = true;
    }
    else {
        // DEBOUNCED OFF
        if ((millis() - _lastDebounceTime) > _debounceDelay) {
            _debouncedState = false;
        }
    }

    return _debouncedState;
}

// Read debounced state without consuming one-shot transition
bool PBSwitch::Read() {
    return UpdateDebounce();
}

// Read button press (one-shot or continuous)
bool PBSwitch::Pressed() {
    bool state = UpdateDebounce();

    bool result;

    if (_oneShot) {
        // Return true only on transition from released to pressed
        result = state && !_lastDebouncedState;
    }
    else {
        // Return true continuously while held
        result = state;
    }

    // Update edge detection state
    _lastDebouncedState = state;

    return result;
}