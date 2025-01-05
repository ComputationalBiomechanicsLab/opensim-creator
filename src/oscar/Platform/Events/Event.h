#pragma once

#include <oscar/Platform/Events/EventType.h>

namespace osc
{
    // base class for application events
    class Event {
    protected:
        Event() = default;
        explicit Event(EventType type) : type_{type} {}
        Event(const Event&) = default;
        Event(Event&&) noexcept = default;
        Event& operator=(const Event&) = default;
        Event& operator=(Event&&) noexcept = default;

    public:
        virtual ~Event() noexcept = default;

        EventType type() const { return type_; }
        bool propagates() const { return propagates_; }
        void enable_propagation() { propagates_ = true; }
        void disable_propagation() { propagates_ = false; }

    private:
        EventType type_ = EventType::Custom;
        bool propagates_ = false;
    };
}
