#pragma once

#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Platform/KeyCombination.h>
#include <liboscar/Platform/KeyModifier.h>
#include <liboscar/Platform/Key.h>

namespace osc
{
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
    private:
        explicit KeyEvent(EventType event_type, KeyCombination combination) :
            Event{event_type},
            combination_{combination}
        {}

        KeyCombination combination_;
    };
}
