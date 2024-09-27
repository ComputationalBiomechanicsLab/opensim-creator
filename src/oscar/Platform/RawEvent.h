#pragma once

#include <oscar/Platform/Event.h>

union SDL_Event;

namespace osc
{
    // a "raw" uncategorized event from the underlying OS/OS abstraction layer
    class RawEvent final : public Event {
    public:
        explicit RawEvent(const SDL_Event& e) :
            Event{EventType::Raw},
            os_event_{&e}
        {}

        const SDL_Event& get_os_event() const { return *os_event_; }
    private:
        const SDL_Event* os_event_ = nullptr;
    };
}
