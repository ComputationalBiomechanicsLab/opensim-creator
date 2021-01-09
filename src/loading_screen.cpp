#include "loading_screen.hpp"

#include "show_model_screen.hpp"
#include "opensim_wrapper.hpp"
#include "application.hpp"

#include "gl.hpp"
#include "imgui.h"

#include <chrono>
#include <string>
#include <future>
#include <vector>
#include <optional>
#include <iostream>
#include <filesystem>


using std::chrono_literals::operator""ms;

namespace osmv {
    struct Loading_screen_impl final {
        std::filesystem::path path;
        std::future<std::optional<osmv::Model>> result;
        std::string error;

        Loading_screen_impl(Application& app, std::filesystem::path const& _path) :
            path{ _path },
            result{std::async(std::launch::async, [&]() {
                return std::optional<osmv::Model>{osmv::load_osim(path)};
            })}
        {
        }

        osmv::Screen_response tick() {
            if (not error.empty()) {
                return Resp_ok{};
            }

            try {
                if (result.wait_for(0ms) == std::future_status::ready) {
                    osmv::Model m = result.get().value();
                    auto show_model_screen = std::make_unique<Show_model_screen>(path, std::move(m));
                    return Resp_transition{std::move(show_model_screen)};
                }
            } catch (std::exception const& ex) {
                error = ex.what();
            }
            return Resp_ok{};
        }

        void draw() {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (error.empty()) {
                bool b = true;
                if (ImGui::Begin("Loading message", &b, ImGuiWindowFlags_MenuBar)) {
                    ImGui::Text("loading: %s", path.c_str());
                    ImGui::End();
                }
            } else {
                bool b = true;
                if (ImGui::Begin("Error loading", &b, ImGuiWindowFlags_MenuBar)) {
                    ImGui::Text("%s", error.c_str());
                    ImGui::End();
                }
            }
        }
    };
}

osmv::Loading_screen::Loading_screen(Application& app, std::filesystem::path const& _path) :
    impl{ new Loading_screen_impl{app, _path} }
{
}
osmv::Loading_screen::~Loading_screen() noexcept = default;


osmv::Screen_response osmv::Loading_screen::tick(Application&) {
    return impl->tick();
}

void osmv::Loading_screen::draw(Application&) {
    impl->draw();
}
