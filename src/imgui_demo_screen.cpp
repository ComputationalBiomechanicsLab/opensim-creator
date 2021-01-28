#include "imgui_demo_screen.hpp"

#include "application.hpp"
#include "splash_screen.hpp"

#include "imgui.h"
#include <SDL.h>

osmv::Event_response osmv::Imgui_demo_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_ESCAPE) {
        request_screen_transition<Splash_screen>();
        return Event_response::handled;
    }

    // osmv::Application already pumps the event into ImGui

    return Event_response::ignored;
}

void osmv::Imgui_demo_screen::draw() {
    // ImGui handles the state of this screen internally

    bool show_demo = true;
    ImGui::ShowDemoWindow(&show_demo);
}
