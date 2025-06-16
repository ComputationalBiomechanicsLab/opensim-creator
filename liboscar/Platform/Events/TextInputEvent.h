#pragma once

#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Platform/Events/EventType.h>
#include <liboscar/Utils/CStringView.h>

#include <string>
#include <utility>

namespace osc
{
    // Represents a "text input event", which can occur when (e.g.) a user
    // uses an on-screen keyboard or operating-system-defined method to input
    // text (e.g. emoji, non-ASCII character).
    class TextInputEvent final : public Event {
    public:
        explicit TextInputEvent(std::string utf8_text) :
            Event{EventType::TextInput},
            utf8_text_{std::move(utf8_text)}
        {}

        CStringView utf8_text() const { return utf8_text_; }
    private:
        std::string utf8_text_;
    };
}
