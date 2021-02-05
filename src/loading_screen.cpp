#include "loading_screen.hpp"

#include "application.hpp"
#include "gl.hpp"
#include "opensim_wrapper.hpp"
#include "show_model_screen.hpp"
#include "splash_screen.hpp"

#include <GL/glew.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <imgui/imgui.h>

#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <utility>

using std::chrono_literals::operator""ms;

namespace osmv {
    struct Loading_screen_impl final {
        std::filesystem::path path;
        std::future<std::optional<osmv::Model>> result;
        std::string error;

        Loading_screen_impl(std::filesystem::path _path) :
            // save the path
            path{std::move(_path)},

            // immediately start loading the model file on a background thread
            result{std::async(std::launch::async, [&]() { return std::optional<osmv::Model>{path}; })} {
        }

        bool on_event(Application& app, SDL_Event const& e) {
            // ESCAPE: go to splash screen
            if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_ESCAPE) {
                app.request_screen_transition<osmv::Splash_screen>();
                return true;
            }

            return false;
        }

        void tick(Application& app) {
            // if there's an error, then the result came through (it's an error)
            // and this screen will just continuously show the error with no
            // recourse
            if (not error.empty()) {
                return;
            }

            // otherwise, there's no error, so the background thread is still
            // loading the osim file.
            try {
                if (result.wait_for(0ms) == std::future_status::ready) {
                    app.request_screen_transition<Show_model_screen>(path, result.get().value());
                    return;
                }
            } catch (std::exception const& ex) {
                error = ex.what();
            }
        }

        void draw() {
            gl::ClearColor(0.99f, 0.98f, 0.96f, 1.0f);
            gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

// PIMPL forwarding

osmv::Loading_screen::Loading_screen(std::filesystem::path _path) : impl{new Loading_screen_impl{std::move(_path)}} {
}

osmv::Loading_screen::~Loading_screen() noexcept {
    delete impl;
}

bool osmv::Loading_screen::on_event(SDL_Event const& e) {
    return impl->on_event(app(), e);
}

void osmv::Loading_screen::tick() {
    return impl->tick(app());
}

void osmv::Loading_screen::draw() {
    impl->draw();
}
