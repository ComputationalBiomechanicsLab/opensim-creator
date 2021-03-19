#include "imgui_demo_screen.hpp"

#include "src/application.hpp"
#include "src/screens/splash_screen.hpp"

#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <imgui.h>

using namespace osmv;

bool Imgui_demo_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        Application::current().request_screen_transition<Splash_screen>();
        return true;
    }

    // osmv::Application already pumps the event into ImGui

    return false;
}

void Imgui_demo_screen::draw() {
    // ImGui handles the state of this screen internally

    bool show_demo = true;
    ImGui::ShowDemoWindow(&show_demo);
}
