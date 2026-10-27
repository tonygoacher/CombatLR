#include <Arduino.h>
#include "statemachine/StateEngine.h"

#include <utility>


StateEngine::StateEngine()
: _currentState(0)
{
    _currentState = _states.end();
}


void StateEngine::PostEvent(const IEvent& event)
{
    _eventQueue.push_back(event.Clone());
}


void StateEngine::ScheduleEvent(const IEvent& event, uint32_t timeInMs)
{
    ScheduledEvent scheduled;
    scheduled.event = event.Clone();
    scheduled.dueTime = static_cast<uint32_t>(millis()) + timeInMs;
    _scheduledEvents.push_back(std::move(scheduled));
}


void StateEngine::CancelEvent(int eventType)
{
    for (auto iter = _eventQueue.begin(); iter != _eventQueue.end();)
    {
        if (*iter && (*iter)->GetType() == eventType)
        {
            iter = _eventQueue.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    for (auto iter = _scheduledEvents.begin(); iter != _scheduledEvents.end();)
    {
        if (iter->event && iter->event->GetType() == eventType)
        {
            iter = _scheduledEvents.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}


void StateEngine::CancelAllEvents()
{
    _eventQueue.clear();
    _scheduledEvents.clear();
}


void StateEngine::PromoteDueEvents(uint32_t now)
{
    // Signed subtraction makes this comparison safe across millis() rollover,
    // provided scheduled delays are less than 2^31 milliseconds.
    for (auto iter = _scheduledEvents.begin(); iter != _scheduledEvents.end();)
    {
        if (static_cast<int32_t>(now - iter->dueTime) >= 0)
        {
            _eventQueue.push_back(std::move(iter->event));
            iter = _scheduledEvents.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}


std::unique_ptr<IEvent> StateEngine::GetEvent()
{
    PromoteDueEvents(static_cast<uint32_t>(millis()));

    if (_eventQueue.empty())
    {
        return nullptr;
    }

    std::unique_ptr<IEvent> event = std::move(_eventQueue.front());
    _eventQueue.pop_front();
    return event;
}


void StateEngine::Run()
{
    if(_currentState != _states.end())
    {
        uint16_t nextState = _currentState->second.Run();
        if(nextState != _currentState->first)
        {

            StateMap::iterator iterNextState = _states.find(nextState);
            if(iterNextState == _states.end())
            {
                Serial.printf("FATAL: StateEngine no state %d found\n", nextState);
                while(1){};
            }

            Serial.printf("Exiting state %d\n", _currentState->first);
            _currentState->second.Exit();


            _currentState = iterNextState;
            Serial.printf("Entering state %d\n", nextState);
            _currentState->second.Entry();
        }
    }
    else
    {
        Serial.println("StateEngine::Run Current is invalid");
    }
}
