#include "loading_screen.hpp"

#include "src/3d/gl.hpp"
#include "src/application.hpp"
#include "src/resources.hpp"
#include "src/screens/model_editor_screen.hpp"
#include "src/screens/splash_screen.hpp"
#include "src/main_editor_state.hpp"

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

using namespace osc;
using std::chrono_literals::operator""ms;

struct Loading_screen::Impl final {
    // filesystem path to the osim being loaded
    std::filesystem::path path;

    // future that lets the UI thread poll the loading thread for
    // the loaded model
    std::future<std::unique_ptr<OpenSim::Model>> result;

    // if not empty, any error encountered by the loading thread
    std::string error;

    // if not nullptr, a main state that should be updated/recycled
    // by this screen when transitioning into the editor
    std::shared_ptr<Main_editor_state> editor_state;

    Impl(std::filesystem::path _path, std::shared_ptr<Main_editor_state> _editor_state) :
        // save the path being loaded
        path{std::move(_path)},

        // immediately start loading the model file on a background thread
        result{std::async(std::launch::async, [&]() { return std::make_unique<OpenSim::Model>(path.string()); })},

        // error is blank until the UI thread encounters an error polling `result`
        error{},

        // save the editor state (if any): it will be forwarded after loading the model
        editor_state{std::move(_editor_state)} {
    }

    Impl(std::filesystem::path _path) : Impl{std::move(_path), nullptr} {
    }
};

static bool on_event(Loading_screen::Impl&, SDL_Event const& e) {
    // ESCAPE: go to splash screen
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        Application::current().request_screen_transition<Splash_screen>();
        return true;
    }

    return false;
}

static void tick(Loading_screen::Impl& impl) {
    // if there's an error, then the result came through (it's an error)
    // and this screen should just continuously show the error
    if (!impl.error.empty()) {
        return;
    }

    // otherwise, poll for the result
    std::unique_ptr<OpenSim::Model> result = nullptr;
    try {
        if (impl.result.wait_for(0ms) == std::future_status::ready) {
            result = impl.result.get();
        }
    } catch (std::exception const& ex) {
        impl.error = ex.what();
        return;
    } catch (...) {
        impl.error = "an unknown exception (does not inherit from std::exception) occurred when loading the file";
        return;
    }

    if (result) {
        add_recent_file(impl.path);

        if (impl.editor_state) {
            // there is an existing editor state
            //
            // recycle it so that users can keep their running sims, local edits, etc.
            impl.editor_state->edited_model = osc::Undoable_ui_model{std::move(result)};
            Application::current().request_screen_transition<Model_editor_screen>(std::move(impl.editor_state));
        } else {
            // there is no existing editor state
            //
            // transitiong into "fresh" editor
            Application::current().request_screen_transition<Model_editor_screen>(std::move(result));
        }
    }
}

static void draw(Loading_screen::Impl& impl) {
    gl::ClearColor(0.99f, 0.98f, 0.96f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (impl.error.empty()) {
        bool b = true;
        if (ImGui::Begin("Loading message", &b, ImGuiWindowFlags_MenuBar)) {
            ImGui::Text("loading: %s", impl.path.string().c_str());
        }
        ImGui::End();
    } else {
        bool b = true;
        if (ImGui::Begin("Error loading", &b, ImGuiWindowFlags_MenuBar)) {
            ImGui::TextUnformatted(impl.error.c_str());
        }
        ImGui::End();
    }
}

// Loading_screen implementation: forwards to IMPL implementations

Loading_screen::Loading_screen(std::filesystem::path _path) :
    impl{new Impl{std::move(_path)}} {
}

Loading_screen::Loading_screen(std::shared_ptr<Main_editor_state> _st, std::filesystem::path _path) :
    impl{new Impl{std::move(_path), std::move(_st)}} {
}

Loading_screen::~Loading_screen() noexcept {
    delete impl;
}

bool Loading_screen::on_event(SDL_Event const& e) {
    return ::on_event(*impl, e);
}

void Loading_screen::tick(float) {
    ::tick(*impl);
}

void Loading_screen::draw() {
    ::draw(*impl);
}
