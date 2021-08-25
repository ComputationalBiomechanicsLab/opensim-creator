#include "main_editor_state.hpp"

#include "src/ui/component_3d_viewer.hpp"

#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <utility>

using namespace osc;

static std::unique_ptr<Component_3d_viewer> create_3dviewer() {
    return std::make_unique<osc::Component_3d_viewer>(osc::Component3DViewerFlags_Default | osc::Component3DViewerFlags_DrawFrames);
}

osc::Main_editor_state::Main_editor_state() :
    Main_editor_state{std::make_unique<OpenSim::Model>()} {
}

osc::Main_editor_state::Main_editor_state(std::unique_ptr<OpenSim::Model> model) :
    edited_model{std::move(model)},
    simulations{},
    focused_simulation{-1},
    focused_simulation_scrubbing_time{-1.0f},
    desired_outputs{},
    sim_params{},
    viewers{create_3dviewer(), nullptr, nullptr, nullptr} {
}

void osc::Main_editor_state::set_model(std::unique_ptr<OpenSim::Model> new_model) {
    edited_model.set_model(std::move(new_model));
}
