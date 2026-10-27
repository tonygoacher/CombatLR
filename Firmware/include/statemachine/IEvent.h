#pragma once

#include <memory>

// Base class for events delivered through a StateEngine.
// Event type values are defined by the state machine that consumes them.
class IEvent
{
public:
    virtual ~IEvent() = default;

    virtual int GetType() const = 0;

    // Return an owned copy of this concrete event.
    virtual std::unique_ptr<IEvent> Clone() const = 0;
};
