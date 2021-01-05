#include "loading_screen.hpp"

#include "show_model_screen.hpp"
#include "opensim_wrapper.hpp"

#include "gl.hpp"
#include "imgui.h"

#include <chrono>
#include <string>
#include <future>
#include <vector>
#include <optional>


using std::chrono_literals::operator""ms;

namespace osmv {
    struct Loading_screen_impl final {
        std::string path;
        std::future<std::optional<osmv::Model>> result;

        Loading_screen_impl(const char* _path) : 
            path{ _path },
            result{ std::async(std::launch::async, [&]() {
                return std::optional<osmv::Model>{osmv::load_osim(path.c_str())};
            }) }
        {
        }

        osmv::Screen_response tick() {
            if (result.wait_for(0ms) == std::future_status::ready) {
                return Resp_transition{ std::make_unique<Show_model_screen>(path, *result.get()) };
            }
            else {
                return Resp_ok{};
            }
        }

        void draw() {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            bool b = true;
            if (ImGui::Begin("Loading message", &b, ImGuiWindowFlags_MenuBar)) {
                ImGui::Text("loading: %s", path.c_str());
                ImGui::End();
            }
        }
    };
}

osmv::Loading_screen::Loading_screen(const char* _path) :
    impl{ new Loading_screen_impl{_path} }
{
}

osmv::Loading_screen::~Loading_screen() noexcept {
    delete impl;
}

osmv::Screen_response osmv::Loading_screen::tick(Application&) {
    return impl->tick();
}

void osmv::Loading_screen::draw(Application&) {
    impl->draw();
}
