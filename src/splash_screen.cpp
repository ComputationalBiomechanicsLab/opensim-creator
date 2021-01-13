#include "splash_screen.hpp"

#include "cfg.hpp"
#include "application.hpp"
#include "loading_screen.hpp"

#include "imgui.h"

namespace osmv {
    struct Splash_screen_impl final {
        std::unique_ptr<osmv::Loading_screen> next_screen = nullptr;

        Screen_response handle_event(Application&, SDL_Event&) {
            return Resp_ok{};
        }

        Screen_response tick(Application&) {
            if (next_screen) {
                return Resp_transition{std::move(next_screen)};
            }
            return Resp_ok{};
        }

        void draw(Application& ui) {
            glm::vec2 app_window_dims = ui.window_size();
            glm::vec2 rough_menu_dims = {250, 250};

            // center the menu
            ImGui::SetNextWindowPos(0.5f * (app_window_dims - rough_menu_dims));
            ImGui::SetNextWindowSize(rough_menu_dims);

            bool b = true;
            if (ImGui::Begin("Loading message", &b, ImGuiWindowFlags_NoTitleBar)) {
                if (ImGui::Button("Rajagopal")) {
                    std::filesystem::path models_dir{"models"};
                    std::filesystem::path demo_model = osmv::cfg::resource_path(models_dir / "ToyLandingModel.osim");
                    next_screen = std::make_unique<osmv::Loading_screen>(ui, demo_model);
                }
                ImGui::End();
            }
        }
    };
}

osmv::Splash_screen::Splash_screen() : impl{new Splash_screen_impl{}} {
}

osmv::Splash_screen::~Splash_screen() noexcept = default;

osmv::Screen_response osmv::Splash_screen::handle_event(Application& app, SDL_Event& e) {
    return impl->handle_event(app, e);
}

osmv::Screen_response osmv::Splash_screen::tick(Application& app) {
    return impl->tick(app);
}

void osmv::Splash_screen::draw(Application& app) {
    impl->draw(app);
}
