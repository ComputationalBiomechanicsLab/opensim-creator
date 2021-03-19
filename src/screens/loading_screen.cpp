#include "loading_screen.hpp"

#include "src/3d/gl.hpp"
#include "src/application.hpp"
#include "src/config.hpp"
#include "src/screens/show_model_screen.hpp"
#include "src/screens/splash_screen.hpp"

#include <GL/glew.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <imgui/imgui.h>

#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <utility>

using namespace osmv;
using std::chrono_literals::operator""ms;

struct Loading_screen::Impl final {
    std::filesystem::path path;
    std::future<std::unique_ptr<OpenSim::Model>> result;
    std::string error;

    Impl(std::filesystem::path _path) :
        // save the path
        path{std::move(_path)},

        // immediately start loading the model file on a background thread
        result{std::async(std::launch::async, [&]() { return std::make_unique<OpenSim::Model>(path.string()); })} {

        OSMV_GL_ASSERT_ALWAYS_NO_GL_ERRORS_HERE("after initializing loading screen");
    }
};

Loading_screen::Loading_screen(std::filesystem::path _path) : impl{new Impl{std::move(_path)}} {
}

Loading_screen::~Loading_screen() noexcept {
    delete impl;
}

bool Loading_screen::on_event(SDL_Event const& e) {
    // ESCAPE: go to splash screen
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        Application::current().request_screen_transition<Splash_screen>();
        return true;
    }

    return false;
}

void Loading_screen::tick() {
    // if there's an error, then the result came through (it's an error)
    // and this screen will just continuously show the error with no
    // recourse
    if (!impl->error.empty()) {
        return;
    }

    // otherwise, there's no error, so the background thread is still
    // loading the osim file.
    try {
        if (impl->result.wait_for(0ms) == std::future_status::ready) {
            config::add_recent_file(impl->path);
            Application::current().request_screen_transition<Show_model_screen>(impl->result.get());
            return;
        }
    } catch (std::exception const& ex) {
        impl->error = ex.what();
    }
}

void Loading_screen::draw() {
    gl::ClearColor(0.99f, 0.98f, 0.96f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (impl->error.empty()) {
        bool b = true;
        if (ImGui::Begin("Loading message", &b, ImGuiWindowFlags_MenuBar)) {
            ImGui::Text("loading: %s", impl->path.c_str());
            ImGui::End();
        }
    } else {
        bool b = true;
        if (ImGui::Begin("Error loading", &b, ImGuiWindowFlags_MenuBar)) {
            ImGui::Text("%s", impl->error.c_str());
            ImGui::End();
        }
    }
}
