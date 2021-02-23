#include "error_screen.hpp"

#include "splash_screen.hpp"
#include "src/application.hpp"

#include <imgui.h>

#include <string>

namespace osmv {
    struct Error_screen_impl final {
        std::string msg;
    };
}

osmv::Error_screen::Error_screen(std::exception const& ex) : impl{new Error_screen_impl{ex.what()}} {
}

osmv::Error_screen::~Error_screen() noexcept {
    delete impl;
}

bool osmv::Error_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_ESCAPE) {
        app().request_screen_transition<Splash_screen>();
        return true;
    }
    return false;
}

void osmv::Error_screen::draw() {
    if (ImGui::Begin("fatal exception")) {
        ImGui::Text("The application threw an exception with the following message:");
        ImGui::Dummy(ImVec2{0.0f, 10.0f});
        ImGui::Text("%s", impl->msg.c_str());
        ImGui::Dummy(ImVec2{0.0f, 10.0f});
        ImGui::Text("(press ESC to return to the splash screen)");
    }
    ImGui::End();
}
