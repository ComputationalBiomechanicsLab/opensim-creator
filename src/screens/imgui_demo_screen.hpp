#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>

// show the official ImGui demo inside the osc UI so that devs can see what widgets are available
namespace osc {
    class Imgui_demo_screen final : public Screen {
    public:
        bool on_event(SDL_Event const&) override;
        void draw() override;
    };
}
