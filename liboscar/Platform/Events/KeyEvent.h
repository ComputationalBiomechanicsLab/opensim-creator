#pragma once

#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Platform/KeyModifier.h>
#include <liboscar/Platform/Key.h>

namespace osc
{
    class KeyEvent final : public Event {
    public:
        static KeyEvent key_up(KeyModifiers modifiers, Key key)
        {
            return KeyEvent{EventType::KeyUp, modifiers, key};
        }

        static KeyEvent key_down(KeyModifiers modifiers, Key key)
        {
            return KeyEvent{EventType::KeyDown, modifiers, key};
        }

        KeyModifiers modifiers() const { return modifiers_; }
        Key key() const { return key_; }

        bool has_modifier(KeyModifier modifier) const { return static_cast<bool>(modifier & modifiers_); }
        bool matches(Key key) const { return key == key_; }
        bool matches(KeyModifier modifier, Key key) const { return modifiers_ & modifier and key == key_; }
        bool matches(KeyModifier modifier1, KeyModifier modifier2, Key key) const { return (modifiers_ & modifier1) and (modifiers_ & modifier2) and key == key_; }
        bool matches(KeyModifiers modifiers, Key key) const { return modifiers == modifiers_ and key == key_; }
    private:
        explicit KeyEvent(EventType event_type, KeyModifiers modifiers, Key key) :
            Event{event_type},
            modifiers_{modifiers},
            key_{key}
        {}

        KeyModifiers modifiers_;
        Key key_;
    };
}
