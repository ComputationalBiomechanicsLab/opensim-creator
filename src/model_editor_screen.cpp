#include "model_editor_screen.hpp"

#include "application.hpp"
#include "simple_model_renderer.hpp"
#include "splash_screen.hpp"

#include <OpenSim/Simulation/Model/Model.h>

namespace osmv {
    struct Model_editor_screen_impl final {
        OpenSim::Model model;
        SimTK::State state;
        Simple_model_renderer renderer;

        Model_editor_screen_impl() :
            model{},
            state{model.initSystem()},
            renderer{app().window_dimensions().w, app().window_dimensions().h, app().samples()} {
        }
    };
}

// PIMPL forwarding

osmv::Model_editor_screen::Model_editor_screen() : impl{new Model_editor_screen_impl{}} {
}

osmv::Model_editor_screen::~Model_editor_screen() noexcept {
    delete impl;
}

bool osmv::Model_editor_screen::on_event(SDL_Event const& e) {
    if (e.type == SDL_KEYDOWN and e.key.keysym.sym == SDLK_ESCAPE) {
        app().request_screen_transition<Splash_screen>();
        return true;
    }
    return impl->renderer.on_event(e);
}

void osmv::Model_editor_screen::tick() {
}

void osmv::Model_editor_screen::draw() {
    impl->renderer.draw(impl->model, impl->state);
}
