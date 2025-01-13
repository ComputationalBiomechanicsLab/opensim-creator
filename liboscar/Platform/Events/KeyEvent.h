#pragma once

#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Platform/KeyModifier.h>
#include <liboscar/Platform/Key.h>

namespace osc
{
    class KeyEvent final : public Event {
    public:
        static KeyEvent key_up(KeyModifier modifier, Key key)
        {
            return KeyEvent{EventType::KeyUp, modifier, key};
        }

        static KeyEvent key_down(KeyModifier modifier, Key key)
        {
            return KeyEvent{EventType::KeyDown, modifier, key};
        }

        KeyModifier modifier() const { return modifier_; }
        Key key() const { return key_; }

        bool matches(Key key) const { return key == key_; }
        bool matches(KeyModifier modifier, Key key) const { return (modifier & modifier_) and (key == key_); }
        bool matches(KeyModifier modifier1, KeyModifier modifier2, Key key) const { return (modifier1 & modifier_) and (modifier2 & modifier_) and (key == key_); }
    private:
        explicit KeyEvent(EventType event_type, KeyModifier key_modifier, Key key) :
            Event{event_type},
            modifier_{key_modifier},
            key_{key}
        {}

        KeyModifier modifier_;
        Key key_;
    };
}
