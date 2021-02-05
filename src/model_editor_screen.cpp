#include "model_editor_screen.hpp"

#include "simple_model_renderer.hpp"
#include "application.hpp"
#include "splash_screen.hpp"

#include <OpenSim/Simulation/Model/Model.h>

namespace osmv {
    struct Model_editor_screen_impl final {
        OpenSim::Model model;
        SimTK::State state;
        Simple_model_renderer renderer;

        Model_editor_screen_impl(Application const& app) :
            model{},
            state{model.initSystem()},
            renderer{app.window_dimensions().w, app.window_dimensions().h, app.samples()}
        {
        }
    };
}

// PIMPL forwarding

osmv::Model_editor_screen::Model_editor_screen(osmv::Application const& app) :
    impl{std::make_unique<Model_editor_screen_impl>(app)} {
}

osmv::Model_editor_screen::~Model_editor_screen() noexcept = default;

osmv::Event_response osmv::Model_editor_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_ESCAPE) {
        request_screen_transition<Splash_screen>();
        return Event_response::handled;
    }
    return impl->renderer.on_event(application(), e) ? Event_response::handled : Event_response::ignored;
}

void osmv::Model_editor_screen::tick() {
}

void osmv::Model_editor_screen::draw() {
    impl->renderer.draw(application(), impl->model, impl->state);
}
