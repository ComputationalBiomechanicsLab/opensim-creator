#pragma once

#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/Key.h>
#include <oscar/Shims/Cpp23/utility.h>

#include <filesystem>
#include <memory>

union SDL_Event;

namespace osc
{
    enum class EventType {
        Raw,
        DropFile,
        KeyDown,
        KeyUp,
        MouseButtonDown,
        MouseButtonUp,
        MouseMove,
        MouseWheel,
        Quit,
        NUM_OPTIONS,
    };

    // base class for application events
    class Event {
    protected:
        explicit Event(const SDL_Event& e, EventType type) :
            inner_event_{&e},
            type_{type}
        {}

        Event(const Event&) = default;
        Event(Event&&) noexcept = default;
        Event& operator=(const Event&) = default;
        Event& operator=(Event&&) noexcept = default;
    public:
        virtual ~Event() noexcept = default;

        EventType type() const { return type_; }

        operator const SDL_Event& () const { return *inner_event_; }
    private:
        const SDL_Event* inner_event_;
        EventType type_;
    };

    // a "raw" uncategorized event from the underlying OS/OS abstraction layer
    class RawEvent final : public Event {
    public:
        explicit RawEvent(const SDL_Event& e) : Event{e, EventType::Raw} {}
    };

    class DropFileEvent final : public Event {
    public:
        explicit DropFileEvent(const SDL_Event&);

        const std::filesystem::path& path() const { return path_; }

    private:
        std::filesystem::path path_;
    };

    enum class KeyModifier : unsigned {
        None         = 0,
        LeftShift    = 1<<0,
        RightShift   = 1<<1,
        LeftCtrl     = 1<<2,
        RightCtrl    = 1<<3,
        LeftGui      = 1<<4,  // Windows key / MacOS command key / Ubuntu Key, etc.
        RightGui     = 1<<5,  // Windows key / MacOS command key / Ubuntu Key, etc.
        LeftAlt      = 1<<6,
        RightAlt     = 1<<7,
        NUM_FLAGS    =    8,

        Ctrl = LeftCtrl | RightCtrl,
        Shift = LeftShift | RightShift,
        Gui = LeftGui | RightGui,
        Alt = LeftAlt | RightAlt,
        CtrlORGui = Ctrl | Gui,
    };

    constexpr bool operator&(KeyModifier lhs, KeyModifier rhs)
    {
        return (cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs)) != 0;
    }

    constexpr KeyModifier operator|(KeyModifier lhs, KeyModifier rhs)
    {
        return static_cast<KeyModifier>(cpp23::to_underlying(lhs) | cpp23::to_underlying(rhs));
    }

    constexpr KeyModifier& operator|=(KeyModifier& lhs, KeyModifier rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    class KeyEvent final : public Event {
    public:
        explicit KeyEvent(const SDL_Event&);

        KeyModifier modifier() const { return modifier_; }
        Key key() const { return key_; }

        bool matches(Key key) const { return key == key_; }
        bool matches(KeyModifier modifier, Key key) const { return (modifier & modifier_) and (key == key_); }
        bool matches(KeyModifier modifier1, KeyModifier modifier2, Key key) const { return (modifier1 & modifier_) and (modifier2 & modifier_) and (key == key_); }
    private:
        KeyModifier modifier_;
        Key key_;
    };

    class MouseEvent final : public Event {
    public:
        explicit MouseEvent(const SDL_Event&);

        Vec2 relative_delta() const { return relative_delta_; }
    private:
        Vec2 relative_delta_;
    };

    class QuitEvent final : public Event {
    public:
        explicit QuitEvent(const SDL_Event&);
    };

    class MouseWheelEvent final : public Event {
    public:
        explicit MouseWheelEvent(const SDL_Event&);

        Vec2 delta() const { return delta_; }
    private:
        Vec2 delta_;
    };

    std::unique_ptr<Event> parse_into_event(const SDL_Event&);
}
