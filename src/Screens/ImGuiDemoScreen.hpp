#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // shows the official ImGui demo
    //
    // this is useful or seeing what widgets are available and how they will
    // look in OSC's application stack
    class ImGuiDemoScreen final : public Screen {
    public:
        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void onDraw() override;
    };
}
