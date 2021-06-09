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

// the function that loads the OpenSim model
//
// this is ran on a background thread
static std::unique_ptr<OpenSim::Model> load_opensim_model(std::string path) {
    std::this_thread::sleep_for(std::chrono::seconds{10});
    return std::make_unique<OpenSim::Model>(path);
}

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

    // a fake progress indicator that never quite reaches 100 %
    //
    // this might seem evil, but its main purpose is to ensure the
    // user that *something* is happening - even if that "something"
    // is "the background thread is deadlocked" ;)
    float progress;

    Impl(std::filesystem::path _path, std::shared_ptr<Main_editor_state> _editor_state) :
        // save the path being loaded
        path{std::move(_path)},

        // immediately start loading the model file on a background thread
        result{std::async(std::launch::async, load_opensim_model, path.string())},

        // error is blank until the UI thread encounters an error polling `result`
        error{},

        // save the editor state (if any): it will be forwarded after loading the model
        editor_state{std::move(_editor_state)},

        progress{0.0f} {
    }

    Impl(std::filesystem::path _path) : Impl{std::move(_path), nullptr} {
    }
};

static void tick(Loading_screen::Impl& impl, float dt) {
    // tick the progress up a little bit
    impl.progress += (dt * (1.0f - impl.progress))/2.0f;

    // if there's an error, then the result came through (it's an error)
    // and this screen should just continuously show the error until the
    // user decides to transition back
    if (!impl.error.empty()) {
        return;
    }

    // otherwise, poll for the result and catch any exceptions that bubble
    // up from the background thread
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

    // if there was a result, handle that
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

static constexpr glm::vec2 menu_dims = {512.0f, 512.0f};

static void draw(Loading_screen::Impl& impl) {
    gl::ClearColor(0.99f, 0.98f, 0.96f, 1.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto [w, h] = Application::current().window_dimensionsf();
    glm::vec2 window_dims{w, h};

    // center the menu
    {
        glm::vec2 menu_pos = (window_dims - menu_dims) / 2.0f;
        ImGui::SetNextWindowPos(menu_pos);
    }

    if (impl.error.empty()) {
        if (ImGui::Begin("Loading message")) {
            ImGui::Text("loading: %s", impl.path.string().c_str());
            ImGui::ProgressBar(impl.progress);
        }
        ImGui::End();
    } else {
        if (ImGui::Begin("Error loading")) {
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

void Loading_screen::tick(float dt) {
    ::tick(*impl, dt);
}

void Loading_screen::draw() {
    ::draw(*impl);
}
