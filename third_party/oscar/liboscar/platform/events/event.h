#pragma once

#include <liboscar/platform/events/event_type.h>

namespace osc
{
    // Represents an event, either spontaneous (i.e. from the operating system
    // as a result of actual user/hardware interaction), or synthesized.
    //
    // Events may or may not "propagate", which indicates to parts of the
    // application that if a particular `Widget` does not handle the event
    // it should bubble it up to its parent `Widget` (if applicable).
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
