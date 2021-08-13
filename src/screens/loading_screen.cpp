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

namespace {
    // the function that loads the OpenSim model
    [[nodiscard]] std::unique_ptr<OpenSim::Model> load_opensim_model(std::string path) {
        return std::make_unique<OpenSim::Model>(path);
    }
}

struct Loading_screen::Impl final {

    // filesystem path to the osim being loaded
    std::filesystem::path path;

    // future that lets the UI thread poll the loading thread for
    // the loaded model
    std::future<std::unique_ptr<OpenSim::Model>> result;

    // if not empty, any error encountered by the loading thread
    std::string error;

    // a main state that should be recycled by this screen when
    // transitioning into the editor
    std::shared_ptr<Main_editor_state> mes;

    // a fake progress indicator that never quite reaches 100 %
    //
    // this might seem evil, but its main purpose is to ensure the
    // user that *something* is happening - even if that "something"
    // is "the background thread is deadlocked" ;)
    float progress;

    Impl(std::filesystem::path _path, std::shared_ptr<Main_editor_state> _mes) :

        // save the path being loaded
        path{std::move(_path)},

        // immediately start loading the model file on a background thread
        result{std::async(std::launch::async, load_opensim_model, path.string())},

        // error is blank until the UI thread encounters an error polling `result`
        error{},

        // save the editor state (if any): it will be forwarded after loading the model
        mes{std::move(_mes)},

        progress{0.0f} {

        OSC_ASSERT(mes != nullptr);
    }
};

// private impl details for loading screen
namespace {
    [[nodiscard]] bool loadingscreen_on_event(Loading_screen::Impl&, SDL_Event const& e) {
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            Application::current().request_transition<Splash_screen>();
            return true;
        }

        return false;
    }

    void loadingscreen_tick(Loading_screen::Impl& impl, float dt) {
        // tick the progress bar up a little bit
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
            if (impl.result.wait_for(std::chrono::seconds{0}) == std::future_status::ready) {
                result = impl.result.get();
            }
        } catch (std::exception const& ex) {
            impl.error = ex.what();
            return;
        } catch (...) {
            impl.error = "an unknown exception (does not inherit from std::exception) occurred when loading the file";
            return;
        }

        // if there was a result (a newly-loaded model), handle it
        if (result) {

            // add newly-loaded model to the "Recent Files" list
            add_recent_file(impl.path);

            // update main editor state with newly-loaded model
            auto uim = std::make_unique<Undoable_ui_model>(std::move(result));
            impl.mes->tabs.emplace_back(std::move(uim));
            impl.mes->cur_tab = static_cast<int>(impl.mes->tabs.size()) - 1;

            // transition to editor
            Application::current().request_transition<Model_editor_screen>(impl.mes);
        }
    }

    void loadingscreen_draw(Loading_screen::Impl& impl) {
        constexpr glm::vec2 menu_dims = {512.0f, 512.0f};

        gl::ClearColor(0.99f, 0.98f, 0.96f, 1.0f);
        gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto [w, h] = Application::current().window_dimensionsf();
        glm::vec2 window_dims{w, h};

        // center the menu
        {
            glm::vec2 menu_pos = (window_dims - menu_dims) / 2.0f;
            ImGui::SetNextWindowPos(menu_pos);
            ImGui::SetNextWindowSize(ImVec2(menu_dims.x, -1));
        }

        if (impl.error.empty()) {
            if (ImGui::Begin("Loading Message", nullptr, ImGuiWindowFlags_NoTitleBar)) {
                ImGui::Text("loading: %s", impl.path.string().c_str());
                ImGui::ProgressBar(impl.progress);
            }
            ImGui::End();
        } else {
            if (ImGui::Begin("Error Message", nullptr, ImGuiWindowFlags_NoTitleBar)) {
                ImGui::TextWrapped("An error occurred while loading the file:");
                ImGui::Dummy(ImVec2{0.0f, 5.0f});
                ImGui::TextWrapped("%s", impl.error.c_str());
                ImGui::Dummy(ImVec2{0.0f, 5.0f});

                if (ImGui::Button("back to splash screen (ESC)")) {
                    Application::current().request_transition<Splash_screen>();
                }
                ImGui::SameLine();
                if (ImGui::Button("try again")) {
                    Application::current().request_transition<Loading_screen>(impl.mes, impl.path);
                }
            }
            ImGui::End();
        }
    }
}

// public API

osc::Loading_screen::Loading_screen(
        std::shared_ptr<Main_editor_state> _st,
        std::filesystem::path _path) :

    impl{new Impl{std::move(_path), std::move(_st)}} {
}

osc::Loading_screen::~Loading_screen() noexcept = default;

bool osc::Loading_screen::on_event(SDL_Event const& e) {
    return ::loadingscreen_on_event(*impl, e);
}

void osc::Loading_screen::tick(float dt) {
    ::loadingscreen_tick(*impl, dt);
}

void osc::Loading_screen::draw() {
    ::loadingscreen_draw(*impl);
}
