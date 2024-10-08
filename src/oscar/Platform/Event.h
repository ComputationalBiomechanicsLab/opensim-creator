#pragma once

#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/EventType.h>
#include <oscar/Platform/Key.h>
#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/CStringView.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

union SDL_Event;
struct SDL_Window;

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

    enum class MouseButton : unsigned {
        None    = 0,     // no button is associated with the `MouseEvent` (e.g. moving the mouse)
        Left    = 1<<0,
        Right   = 1<<1,
        Middle  = 1<<2,
        Back    = 1<<3,  // sometimes called X1 (SDL), ExtraButton1 (Qt)
        Forward = 1<<4,  // sometimes called X2 (SDL), ExtraButton2 (Qt)
    };

    enum class MouseInputSource {
        Mouse,
        TouchScreen,
    };

    class TextInputEvent final : public Event {
    public:
        explicit TextInputEvent(const SDL_Event&);

        CStringView utf8_text() const { return utf8_text_; }
    private:
        std::string utf8_text_;
    };

    // fired off when the state of a display changes, such as:
    //
    // - display connected
    // - display disconnected
    // - display reoriented
    // - display resolution changed (maybe DPI change)
    class DisplayStateChangeEvent final : public Event {
    public:
        explicit DisplayStateChangeEvent(const SDL_Event&);
    };

    enum class WindowEventType {
        GainedMouseFocus,
        LostMouseFocus,
        GainedKeyboardFocus,
        LostKeyboardFocus,
        WindowClosed,
        WindowMoved,
        WindowResized
    };
    class WindowEvent final : public Event {
    public:
        explicit WindowEvent(const SDL_Event&);

        WindowEventType type() const { return type_; }
        const SDL_Window* window() const { return window_; }
        uint32_t window_id() const { return window_id_; }
    private:
        WindowEventType type_ = WindowEventType::WindowMoved;
        SDL_Window* window_ = nullptr;
        uint32_t window_id_ = 0;
    };

    class MouseEvent final : public Event {
    public:
        explicit MouseEvent(const SDL_Event&);

        MouseInputSource input_source() const { return input_source_; }
        MouseButton button() const { return button_; }
        Vec2 relative_delta() const { return relative_delta_; }
        Vec2 position_in_window() const { return position_in_window_; }
    private:
        Vec2 relative_delta_{};
        Vec2 position_in_window_{};
        MouseInputSource input_source_ = MouseInputSource::Mouse;
        MouseButton button_ = MouseButton::None;
    };

    class QuitEvent final : public Event {
    public:
        explicit QuitEvent(const SDL_Event&);
    };

    class MouseWheelEvent final : public Event {
    public:
        explicit MouseWheelEvent(const SDL_Event&);

        MouseInputSource input_source() const { return input_source_; }
        Vec2 delta() const { return delta_; }
    private:
        Vec2 delta_;
        MouseInputSource input_source_ = MouseInputSource::TouchScreen;
    };

    std::unique_ptr<Event> try_parse_into_event(const SDL_Event&);
}
