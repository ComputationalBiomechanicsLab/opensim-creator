#include "error_screen.hpp"

#include "src/application.hpp"
#include "src/screens/splash_screen.hpp"

#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <imgui.h>

#include <stdexcept>
#include <string>

using namespace osmv;

struct Error_screen::Impl final {
    std::string msg;
};

Error_screen::Error_screen(std::exception const& ex) : impl{new Impl{ex.what()}} {
}

Error_screen::~Error_screen() noexcept {
    delete impl;
}

bool Error_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_ESCAPE) {
        Application::current().request_screen_transition<Splash_screen>();
        return true;
    }
    return false;
}

void Error_screen::draw() {
    if (ImGui::Begin("fatal exception")) {
        ImGui::Text("The application threw an exception with the following message:");
        ImGui::Dummy(ImVec2{0.0f, 10.0f});
        ImGui::Text("%s", impl->msg.c_str());
        ImGui::Dummy(ImVec2{0.0f, 10.0f});
        ImGui::Text("(press ESC to return to the splash screen)");
    }
    ImGui::End();
}
