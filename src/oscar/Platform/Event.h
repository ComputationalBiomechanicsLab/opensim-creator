#pragma once

#include <filesystem>
#include <memory>

union SDL_Event;

namespace osc
{
    // base class for application events
    class Event {
    protected:
        explicit Event(const SDL_Event& e) : inner_event_{&e} {}

        Event(const Event&) = default;
        Event(Event&&) noexcept = default;
        Event& operator=(const Event&) = default;
        Event& operator=(Event&&) noexcept = default;
    public:
        virtual ~Event() noexcept = default;

        operator const SDL_Event& () const { return *inner_event_; }
    private:
        const SDL_Event* inner_event_;
    };

    // a "raw" uncategorized event from the underlying OS/OS abstraction layer
    class RawEvent : public Event {
    public:
        explicit RawEvent(const SDL_Event& e) : Event{e} {}
    };

    class DropFileEvent : public Event {
    public:
        explicit DropFileEvent(const SDL_Event& e);

        const std::filesystem::path& path() const { return path_; }

    private:
        std::filesystem::path path_;
    };

    std::unique_ptr<Event> parse_into_event(const SDL_Event&);
}
