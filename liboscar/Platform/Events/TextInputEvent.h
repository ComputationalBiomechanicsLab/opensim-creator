#pragma once

#include <liboscar/Platform/Events/Event.h>
#include <liboscar/Platform/Events/EventType.h>
#include <liboscar/Utils/CStringView.h>

#include <string>
#include <utility>

namespace osc
{
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
