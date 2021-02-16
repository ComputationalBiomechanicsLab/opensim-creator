#include "splash_screen.hpp"

#include "application.hpp"
#include "config.hpp"
#include "gl.hpp"
#include "imgui_demo_screen.hpp"
#include "loading_screen.hpp"
#include "model_editor_screen.hpp"
#include "opengl_test_screen.hpp"
#include "osmv_config.hpp"

#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

static bool filename_lexographically_gt(fs::path const& a, fs::path const& b) {
    return a.filename() < b.filename();
}

// helper that searches for example .osim files in the resources/ directory
static std::vector<fs::path> find_example_osims() {
    fs::path models_dir = osmv::config::resource_path("models");

    std::vector<fs::path> rv;

    if (not fs::exists(models_dir)) {
        // probably running from a weird location, or resources are missing
        return rv;
    }

    if (not fs::is_directory(models_dir)) {
        // something horrible has happened, but gracefully fallback to ignoring
        // that issue (grumble grumble, this should be logged)
        return rv;
    }

    for (fs::directory_entry const& e : fs::recursive_directory_iterator{models_dir}) {
        if (e.path().extension() == ".osim") {
            rv.push_back(e.path());
        }
    }

    std::sort(rv.begin(), rv.end(), filename_lexographically_gt);

    return rv;
}

namespace osmv {
    struct Splash_screen_impl final {
        std::vector<fs::path> example_osims = find_example_osims();
        std::vector<config::Recent_file> recent_files = osmv::config::recent_files();

        bool on_event(Application& app, SDL_Event const& e) {
            if (e.type == SDL_KEYDOWN) {
                SDL_Keycode sym = e.key.keysym.sym;

                // ESCAPE: quit application
                if (sym == SDLK_ESCAPE) {
                    app.request_quit_application();
                    return true;
                }
            }
            return false;
        }

        void draw(Application& app) {
            // center the menu
            {
                static constexpr int menu_width = 700;
                static constexpr int menu_height = 700;

                auto d = app.window_dimensions();
                int menu_x = (d.w - menu_width) / 2;
                int menu_y = (d.h - menu_height) / 2;

                ImGui::SetNextWindowPos(ImVec2(menu_x, menu_y));
                ImGui::SetNextWindowSize(ImVec2{menu_width, -1});
                ImGui::SetNextWindowSizeConstraints({menu_width, menu_height}, {menu_width, menu_height});
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
                    if (not recent_files.empty()) {
                        ImGui::Text("Recent files:");
                        ImGui::Dummy(ImVec2{0.0f, 3.0f});

                        // iterate in reverse: recent files are stored oldest --> newest
                        for (auto it = recent_files.rbegin(); it != recent_files.rend(); ++it) {
                            config::Recent_file const& rf = *it;
                            ImGui::PushID(++id);
                            if (ImGui::Button(rf.path.filename().c_str())) {
                                app.request_screen_transition<osmv::Loading_screen>(rf.path);
                            }
                            ImGui::PopID();
                        }
                    }

                    ImGui::Dummy(ImVec2{0.0f, 5.0f});

                    // examples:
                    if (not example_osims.empty()) {
                        ImGui::Text("Examples:");
                        ImGui::Dummy(ImVec2{0.0f, 3.0f});

                        for (fs::path const& ex : example_osims) {
                            ImGui::PushID(++id);
                            if (ImGui::Button(ex.filename().c_str())) {
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
    };
}

// PIMPL forwarding for osmv::Splash_screen

osmv::Splash_screen::Splash_screen() : impl{new Splash_screen_impl{}} {
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
}

osmv::Splash_screen::~Splash_screen() noexcept {
    delete impl;
}

bool osmv::Splash_screen::on_event(SDL_Event const& e) {
    return impl->on_event(app(), e);
}

void osmv::Splash_screen::draw() {
    impl->draw(app());
}
