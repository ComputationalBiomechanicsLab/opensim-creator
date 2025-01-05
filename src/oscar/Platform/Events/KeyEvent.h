#pragma once

#include <oscar/Platform/Events/Event.h>
#include <oscar/Platform/KeyModifier.h>
#include <oscar/Platform/Key.h>

union SDL_Event;

namespace osc
{
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
}
