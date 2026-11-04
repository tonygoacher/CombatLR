#pragma once

#include "IEvent.h"
#include "State.h"
#include <string>

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <vector>

class StateEngine
{
public:
    StateEngine(const char * name);
    virtual void Start() = 0;
    
    void Run();

    // Events are cloned, so the caller retains ownership of the original.
    void PostEvent(const IEvent& event);

    // Queue a copy of event for delivery after timeInMs has elapsed.
    // Delays must be less than 2^31 milliseconds for rollover-safe timing.
    void ScheduleEvent(const IEvent& event, uint32_t timeInMs);

    // Cancel all queued or scheduled events with this event type.
    void CancelEvent(int eventType);

    // Discard all queued and scheduled events.
    void CancelAllEvents();

    // Transfer ownership of the next available event to the caller.
    // Returns nullptr when no event is ready.
    std::unique_ptr<IEvent> GetEvent();

protected:
    typedef std::pair<uint16_t, State&> StateMapEntry;
    typedef std::map<uint16_t, State&> StateMap;

    struct ScheduledEvent
    {
        std::unique_ptr<IEvent> event;
        uint32_t dueTime;
    };

    void PromoteDueEvents(uint32_t now);

    StateMap _states;
    StateMap::iterator _currentState;

    std::deque<std::unique_ptr<IEvent>> _eventQueue;
    std::vector<ScheduledEvent> _scheduledEvents;

    static const int FirstState = 0;

    std::string _name;
};
