#pragma once

#include <liboscar/platform/events/event.h>
#include <liboscar/platform/key_combination.h>
#include <liboscar/platform/key_modifier.h>
#include <liboscar/platform/key.h>
#include <liboscar/platform/physical_key_modifier.h>

namespace osc
{
    // Represents a single key press (down) or key release (up), possibly
    // while other modifier keys (e.g. ctrl) are also pressed.
    class KeyEvent final : public Event {
    public:
        static KeyEvent key_up(KeyCombination combination)
        {
            return KeyEvent{EventType::KeyUp, combination};
        }

        static KeyEvent key_down(KeyCombination combination)
        {
            return KeyEvent{EventType::KeyDown, combination};
        }

        KeyCombination combination() const { return combination_; }
        KeyModifiers modifiers() const { return combination_.modifiers(); }
        Key key() const { return combination_.key(); }
        bool has_modifier(KeyModifier modifier) const { return static_cast<bool>(modifier & combination_.modifiers()); }
        bool has_modifier(PhysicalKeyModifier modifier) const { return static_cast<bool>(modifier & to<PhysicalKeyModifiers>(combination_.modifiers())); }
    private:
        explicit KeyEvent(EventType event_type, KeyCombination combination) :
            Event{event_type},
            combination_{combination}
        {}

        KeyCombination combination_;
    };
}
