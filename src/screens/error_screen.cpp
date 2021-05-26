#include "error_screen.hpp"

#include "src/application.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/ui/log_viewer.hpp"

#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <imgui.h>

#include <stdexcept>
#include <string>

using namespace osc;

struct Error_screen::Impl final {
    std::string msg;
    ui::log_viewer::State log;

    Impl(std::string _msg) : msg{std::move(_msg)} {
    }
};

Error_screen::Error_screen(std::exception const& ex) : impl{new Impl{ex.what()}} {
}

Error_screen::~Error_screen() noexcept {
    delete impl;
}

bool Error_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        Application::current().request_screen_transition<Splash_screen>();
        return true;
    }
    return false;
}

void Error_screen::draw() {
    constexpr float width = 800.0f;
    constexpr float padding = 10.0f;

    // center error near top of screen
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        center.y = padding;
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(width, 0.0f));
    }

    if (ImGui::Begin("fatal error")) {
        ImGui::TextWrapped("The application threw an exception with the following message:");
        ImGui::Dummy(ImVec2{2.0f, 10.0f});
        ImGui::SameLine();
        ImGui::TextWrapped("%s", impl->msg.c_str());
        ImGui::Dummy(ImVec2{0.0f, 10.0f});

        if (ImGui::Button("Return to splash screen (Escape)")) {
            Application::current().request_screen_transition<Splash_screen>();
        }
    }
    ImGui::End();

    // center log at bottom of screen
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        center.y = ImGui::GetMainViewport()->Size.y - padding;
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(width, 0.0f));
    }

    if (ImGui::Begin("Log", nullptr, ImGuiWindowFlags_MenuBar)) {
        ui::log_viewer::draw(impl->log);
    }
    ImGui::End();
}
