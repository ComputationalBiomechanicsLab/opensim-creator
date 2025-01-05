#pragma once

#include <oscar/Platform/Events/Event.h>
#include <oscar/Utils/CStringView.h>

#include <string>

union SDL_Event;

namespace osc
{
    class TextInputEvent final : public Event {
    public:
        explicit TextInputEvent(const SDL_Event&);

        CStringView utf8_text() const { return utf8_text_; }
    private:
        std::string utf8_text_;
    };
}
