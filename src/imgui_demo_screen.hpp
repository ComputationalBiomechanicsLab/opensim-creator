#pragma once

#include "screen.hpp"

#include <SDL_events.h>

// show the official ImGui demo inside the osmv UI so that devs can see what widgets are available
namespace osmv {
    class Imgui_demo_screen final : public Screen {
    public:
        bool on_event(SDL_Event const&) override;
        void draw() override;
    };
}
