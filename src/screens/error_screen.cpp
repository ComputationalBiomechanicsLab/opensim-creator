#include "error_screen.hpp"

#include "src/app.hpp"
#include "src/ui/log_viewer.hpp"
#include "src/3d/gl.hpp"
#include "src/log.hpp"

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

// public API

osc::Error_screen::Error_screen(std::exception const& ex) : impl{new Impl{ex.what()}} {
}

osc::Error_screen::~Error_screen() noexcept = default;

void osc::Error_screen::on_mount() {
    osc::ImGuiInit();
}

void osc::Error_screen::on_unmount() {
    osc::ImGuiShutdown();
}

void osc::Error_screen::on_event(SDL_Event const& e) {
    if (osc::ImGuiOnEvent(e)) {
        return;
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        // TODO
        // Application::current().request_transition<Splash_screen>();
    }
}

void osc::Error_screen::draw() {
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    osc::ImGuiNewFrame();

    constexpr float width = 800.0f;
    constexpr float padding = 10.0f;

    // error message panel
    {
        glm::vec2 pos{App::cur().dims().x/2.0f, padding};
        ImGui::SetNextWindowPos(pos, ImGuiCond_Once, {0.5f, 0.0f});
        ImGui::SetNextWindowSize({width, 0.0f});

        if (ImGui::Begin("fatal error")) {
            ImGui::TextWrapped("The application threw an exception with the following message:");
            ImGui::Dummy(ImVec2{2.0f, 10.0f});
            ImGui::SameLine();
            ImGui::TextWrapped("%s", impl->msg.c_str());
            ImGui::Dummy(ImVec2{0.0f, 10.0f});

            if (ImGui::Button("Return to splash screen (Escape)")) {
                // TODO
                // Application::current().request_transition<Splash_screen>();
            }
        }
        ImGui::End();
    }

    // log message panel
    {
        glm::vec2 dims = App::cur().dims();
        glm::vec2 pos{dims.x/2.0f, dims.y - padding};
        ImGui::SetNextWindowPos(pos, ImGuiCond_Once, ImVec2(0.5f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(width, 0.0f));

        if (ImGui::Begin("Error Log", nullptr, ImGuiWindowFlags_MenuBar)) {
            ui::log_viewer::draw(impl->log);
        }
        ImGui::End();
    }

    osc::ImGuiRender();
}
