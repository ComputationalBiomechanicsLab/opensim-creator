#include "experimental_merged_screen.hpp"

#include <OpenSim/Simulation/Model/Model.h>

using namespace osmv;

struct Experimental_merged_screen::Impl final {
    std::unique_ptr<OpenSim::Model> model;

    Impl(std::unique_ptr<OpenSim::Model> _model) : model{std::move(_model)} {
    }
};

Experimental_merged_screen::Experimental_merged_screen() : impl{new Impl{std::make_unique<OpenSim::Model>()}} {
}

Experimental_merged_screen::Experimental_merged_screen(std::unique_ptr<OpenSim::Model> model) :
    impl{new Impl{std::move(model)}} {
}

Experimental_merged_screen::~Experimental_merged_screen() noexcept {
    delete impl;
}

bool Experimental_merged_screen::on_event(SDL_Event const&) {
    return false;
}

void Experimental_merged_screen::tick() {
}
void Experimental_merged_screen::draw() {
}
