#include "splash_screen.hpp"

#include "cfg.hpp"
#include "application.hpp"
#include "loading_screen.hpp"

#include "imgui.h"

#include <algorithm>

namespace fs = std::filesystem;

namespace osmv {
    struct Splash_screen_impl final {
        std::vector<fs::path> example_osims = []() {
            fs::path models_dir = osmv::cfg::resource_path("models");

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
        }();

        void on_event(Application& app, SDL_Event& e) {
            if (e.type == SDL_KEYDOWN) {
                SDL_Keycode sym = e.key.keysym.sym;

                // ESCAPE: quit application
                if (sym == SDLK_ESCAPE) {
                    app.request_quit();
                    return;
                }

                // 1-9: load numbered example
                if (SDLK_1 <= sym and sym <= SDLK_9) {
                    size_t idx = static_cast<size_t>(sym - SDLK_1);
                    if (idx < example_osims.size()) {
                        fs::path const& p = example_osims[idx];
                        auto s = std::make_unique<osmv::Loading_screen>(p);
                        app.request_transition(std::move(s));
                        return;
                    }
                }
            }
        }

        void draw(Application& ui) {
            bool b = true;
            std::unique_ptr<osmv::Screen> should_transition_to = nullptr;
            bool should_exit = false;
            char buf[512];

            // center the menu
            {
                glm::vec2 app_window_dims = ui.window_size();
                glm::vec2 rough_menu_dims = {250, 250};
                glm::vec2 menu_pos = 0.5f * (app_window_dims - rough_menu_dims);
                menu_pos.y = 100;
                ImGui::SetNextWindowPos(menu_pos);
                ImGui::SetNextWindowSize(ImVec2{250, -1});
                ImGui::SetNextWindowSizeConstraints({250, 250}, {500, 500});
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
                    std::snprintf(buf, sizeof(buf), "%zu: %s", i + 1, p.filename().c_str());
                    if (ImGui::Button(buf)) {
                        should_transition_to = std::make_unique<osmv::Loading_screen>(p);
                    }
                }

                ImGui::Dummy(ImVec2{0.0f, 10.0f});
                ImGui::Separator();
                ImGui::Dummy(ImVec2{0.0f, 2.0f});

                if (ImGui::Button("Exit")) {
                    should_exit = true;
                }

                ImGui::End();
            }

            if (should_transition_to) {
                ui.request_transition(std::move(should_transition_to));
                return;
            }

            if (should_exit) {
                ui.request_quit();
                return;
            }
        }
    };


    // PIMPL forwarding

    Splash_screen::Splash_screen() : impl{new Splash_screen_impl{}} {
    }

    Splash_screen::~Splash_screen() noexcept = default;

    void Splash_screen::on_event(SDL_Event& e) {
        impl->on_event(application(), e);
    }

    void Splash_screen::draw() {
        impl->draw(application());
    }
}
