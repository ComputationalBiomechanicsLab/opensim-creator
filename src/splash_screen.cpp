#include "splash_screen.hpp"

#include "application.hpp"
#include "config.hpp"
#include "imgui_demo_screen.hpp"
#include "loading_screen.hpp"
#include "model_editor_screen.hpp"

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

    std::sort(rv.begin(), rv.end());

    return rv;
}

namespace osmv {
    struct Splash_screen_impl final {
        std::vector<fs::path> example_osims = find_example_osims();

        Event_response on_event(Application& app, SDL_Event const& e) {
            if (e.type == SDL_KEYDOWN) {
                SDL_Keycode sym = e.key.keysym.sym;

                // ESCAPE: quit application
                if (sym == SDLK_ESCAPE) {
                    app.request_quit_application();
                    return Event_response::handled;
                }

                // 1-9: load numbered example
                if (SDLK_1 <= sym and sym <= SDLK_9) {
                    size_t idx = static_cast<size_t>(sym - SDLK_1);
                    if (idx < example_osims.size()) {
                        app.request_screen_transition<Loading_screen>(example_osims[idx]);
                        return Event_response::handled;
                    }
                }
            }
            return Event_response::ignored;
        }

        void draw(Application& app) {
            bool b = true;
            std::unique_ptr<osmv::Screen> should_transition_to = nullptr;
            bool should_exit = false;
            char buf[512];

            // center the menu
            {
                auto [w, h] = app.window_dimensions();
                glm::vec2 app_window_dims{w, h};
                glm::vec2 rough_menu_dims = {500, 500};
                glm::vec2 menu_pos = 0.5f * (app_window_dims - rough_menu_dims);
                menu_pos.y = 100;

                ImGui::SetNextWindowPos(menu_pos);
                ImGui::SetNextWindowSize(ImVec2{500, -1});
                ImGui::SetNextWindowSizeConstraints({500, 500}, {500, 500});
            }

            if (ImGui::Begin("Splash screen", &b, ImGuiWindowFlags_NoTitleBar)) {

                ImGui::Text("OpenSim Model Viewer (osmv)");
                ImGui::Dummy(ImVec2{0.0f, 5.0f});

                ImGui::Text("For development use");
                ImGui::Dummy(ImVec2{0.0f, 5.0f});

                if (not example_osims.empty()) {
                    ImGui::Text("Examples");
                    ImGui::Separator();
                    ImGui::Dummy(ImVec2{0.0f, 3.0f});
                }

                // list of examples with buttons
                for (size_t i = 0; i < example_osims.size(); ++i) {
                    fs::path const& p = example_osims[i];
                    std::snprintf(buf, sizeof(buf), "%zu: %s", i + 1, p.filename().string().c_str());
                    if (ImGui::Button(buf)) {
                        should_transition_to = std::make_unique<osmv::Loading_screen>(p);
                    }
                }

                ImGui::Dummy(ImVec2{0.0f, 10.0f});
                ImGui::Separator();
                ImGui::Dummy(ImVec2{0.0f, 2.0f});

                if (ImGui::Button("ImGui demo")) {
                    should_transition_to = std::make_unique<osmv::Imgui_demo_screen>();
                }

                if (ImGui::Button("editor")) {
                    should_transition_to = std::make_unique<osmv::Model_editor_screen>(app);
                }

                if (ImGui::Button("Exit")) {
                    should_exit = true;
                }

                ImGui::End();
            }

            if (should_transition_to) {
                app.request_screen_transition(std::move(should_transition_to));
                return;
            }

            if (should_exit) {
                app.request_quit_application();
                return;
            }
        }
    };
}

// PIMPL forwarding for osmv::Splash_screen

osmv::Splash_screen::Splash_screen() : impl{new Splash_screen_impl{}} {
}

osmv::Splash_screen::~Splash_screen() noexcept = default;

osmv::Event_response osmv::Splash_screen::on_event(SDL_Event const& e) {
    return impl->on_event(application(), e);
}

void osmv::Splash_screen::draw() {
    impl->draw(application());
}
