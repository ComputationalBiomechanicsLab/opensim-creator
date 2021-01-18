#pragma once

#include "screen.hpp"

// show the official ImGui demo inside the osmv UI so that devs can see what widgets are available
namespace osmv {
    class Imgui_demo_screen final : public Screen {
    public:
        Event_response on_event(SDL_Event const&) override;
        void draw() override;
    };
}
