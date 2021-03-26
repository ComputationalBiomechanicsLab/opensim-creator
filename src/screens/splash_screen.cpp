#include "splash_screen.hpp"

#include "osmv_config.hpp"
#include "src/3d/gl.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/screens/experimental_merged_screen.hpp"
#include "src/screens/imgui_demo_screen.hpp"
#include "src/screens/loading_screen.hpp"
#include "src/screens/model_editor_screen.hpp"
#include "src/screens/opengl_test_screen.hpp"
#include "src/utils/geometry.hpp"
#include "src/utils/scope_guard.hpp"
#include "src/widgets/main_menu_about_tab.hpp"
#include "src/widgets/main_menu_file_tab.hpp"

#include <GL/glew.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace osmv;

struct Splash_screen::Impl final {
    Main_menu_file_tab_state mm_state;
};

// PIMPL forwarding for osmv::Splash_screen

osmv::Splash_screen::Splash_screen() : impl{new Impl{}} {
}

osmv::Splash_screen::~Splash_screen() noexcept {
    delete impl;
}

static bool on_keydown(SDL_KeyboardEvent const& e) {
    SDL_Keycode sym = e.keysym.sym;

    if (e.keysym.mod & KMOD_CTRL) {
        // CTRL

        switch (sym) {
        case SDLK_n:
            main_menu_new();
            return true;
        case SDLK_o:
            main_menu_open();
            return true;
        case SDLK_q:
            Application::current().request_quit_application();
            return true;
        }
    }

    return false;
}

bool osmv::Splash_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN) {
        return on_keydown(e.key);
    }
    return false;
}

void osmv::Splash_screen::draw() {
    Application& app = Application::current();

    if (ImGui::BeginMainMenuBar()) {
        draw_main_menu_file_tab(impl->mm_state);
        draw_main_menu_about_tab();
        ImGui::EndMainMenuBar();
    }

    // center the menu
    {
        static constexpr int menu_width = 700;
        static constexpr int menu_height = 700;

        auto d = app.window_dimensions();
        float menu_x = static_cast<float>((d.w - menu_width) / 2);
        float menu_y = static_cast<float>((d.h - menu_height) / 2);

        ImGui::SetNextWindowPos(ImVec2(menu_x, menu_y));
        ImGui::SetNextWindowSize(ImVec2(menu_width, -1));
        ImGui::SetNextWindowSizeConstraints(ImVec2(menu_width, menu_height), ImVec2(menu_width, menu_height));
    }

    bool b = true;
    if (ImGui::Begin("Splash screen", &b, ImGuiWindowFlags_NoTitleBar)) {
        ImGui::Columns(2);

        // left-column: utils etc.
        {
            ImGui::Text("Utilities:");
            ImGui::Dummy(ImVec2{0.0f, 3.0f});

            if (ImGui::Button("ImGui demo")) {
                app.request_screen_transition<osmv::Imgui_demo_screen>();
            }

            if (ImGui::Button("Model editor")) {
                app.request_screen_transition<osmv::Model_editor_screen>();
            }

            if (ImGui::Button("Rendering tests (meta)")) {
                app.request_screen_transition<osmv::Opengl_test_screen>();
            }

            if (ImGui::Button("experimental new merged screen")) {
                auto rajagopal_path = config::resource_path("models", "RajagopalModel", "Rajagopal2015.osim");
                auto model = std::make_unique<OpenSim::Model>(rajagopal_path.string());
                app.request_screen_transition<Experimental_merged_screen>(std::move(model));
            }

            ImGui::Dummy(ImVec2{0.0f, 4.0f});
            if (ImGui::Button("Exit")) {
                app.request_quit_application();
            }

            ImGui::NextColumn();
        }

        // right-column: open, recent files, examples
        {
            // de-dupe imgui IDs because these lists may contain duplicate
            // names
            int id = 0;

            // recent files:
            if (!impl->mm_state.recent_files.empty()) {
                ImGui::Text("Recent files:");
                ImGui::Dummy(ImVec2{0.0f, 3.0f});

                // iterate in reverse: recent files are stored oldest --> newest
                for (auto it = impl->mm_state.recent_files.rbegin(); it != impl->mm_state.recent_files.rend(); ++it) {
                    config::Recent_file const& rf = *it;
                    ImGui::PushID(++id);
                    if (ImGui::Button(rf.path.filename().string().c_str())) {
                        app.request_screen_transition<osmv::Loading_screen>(rf.path);
                    }
                    ImGui::PopID();
                }
            }

            ImGui::Dummy(ImVec2{0.0f, 5.0f});

            // examples:
            if (!impl->mm_state.example_osims.empty()) {
                ImGui::Text("Examples:");
                ImGui::Dummy(ImVec2{0.0f, 3.0f});

                for (fs::path const& ex : impl->mm_state.example_osims) {
                    ImGui::PushID(++id);
                    if (ImGui::Button(ex.filename().string().c_str())) {
                        app.request_screen_transition<osmv::Loading_screen>(ex);
                    }
                    ImGui::PopID();
                }
            }

            ImGui::NextColumn();
        }
    }
    ImGui::End();

    // bottom-right: version info etc.
    {
        auto wd = app.window_dimensions();
        ImVec2 wd_imgui = {static_cast<float>(wd.w), static_cast<float>(wd.h)};

        char const* l1 = "osmv " OSMV_VERSION_STRING;
        ImVec2 l1_dims = ImGui::CalcTextSize(l1);

        char buf[512];
        std::snprintf(
            buf,
            sizeof(buf),
            "OpenGL: %s, %s (%s); GLSL %s",
            glGetString(GL_VENDOR),
            glGetString(GL_RENDERER),
            glGetString(GL_VERSION),
            glGetString(GL_SHADING_LANGUAGE_VERSION));
        ImVec2 l2_dims = ImGui::CalcTextSize(buf);

        ImVec2 l2_pos = {0, wd_imgui.y - l2_dims.y};
        ImVec2 l1_pos = l2_pos;
        l1_pos.y -= l1_dims.y;

        ImGui::GetBackgroundDrawList()->AddText(l1_pos, 0xaaaaaaaa, l1);
        ImGui::GetBackgroundDrawList()->AddText(l2_pos, 0xaaaaaaaa, buf);
    }
}
