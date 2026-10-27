#pragma once

#include "IEvent.h"

// Lightweight event carrying only an integer type/id.
class GenericEvent : public IEvent
{
public:
    explicit GenericEvent(int type)
        : _type(type)
    {
    }

    int GetType() const override
    {
        return _type;
    }

    std::unique_ptr<IEvent> Clone() const override
    {
        return std::make_unique<GenericEvent>(*this);
    }

private:
    int _type;
};
