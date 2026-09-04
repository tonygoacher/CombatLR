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
}

bool PBSwitch::RawRead() {
    if (_port == 255) return false;
    
    // Assumes INPUT_PULLUP (LOW means the button is pressed down)
    return (digitalRead(_port) == LOW); 
}

bool PBSwitch::Pressed() {
    if (_port == 255) return false;

    // Read the current physical state of the pin (LOW = pressed)
    bool currentRawPressed = (digitalRead(_port) == LOW);

    if (currentRawPressed) {
        // IMMEDIATE ON: Register as pressed immediately and reset the debounce timer
        _lastDebounceTime = millis();
        _debouncedState = true;
    } else {
        // DEBOUNCED OFF: Only register as released if it has been physically 
        // unpressed for longer than the debounce delay
        if ((millis() - _lastDebounceTime) > _debounceDelay) {
            _debouncedState = false;
        }
    }

    bool isPressed = false;

    if (_oneShot) {
        // Return true ONLY on the exact transition from unpressed to pressed
        if (_debouncedState && !_lastDebouncedState) {
            isPressed = true;
        }
    } else {
        // Return true continuously as long as it is considered held down
        isPressed = _debouncedState;
    }

    // Save the debounced state for the next cycle's edge detection
    _lastDebouncedState = _debouncedState;

    return isPressed;
}