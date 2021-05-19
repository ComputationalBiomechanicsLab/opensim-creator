#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>

namespace osc {

    // shows the official ImGui demo
    //
    // this is useful or seeing what widgets are available and how they will
    // look in OSC's application stack
    class Imgui_demo_screen final : public Screen {
    public:
        bool on_event(SDL_Event const&) override;
        void draw() override;
    };
}
