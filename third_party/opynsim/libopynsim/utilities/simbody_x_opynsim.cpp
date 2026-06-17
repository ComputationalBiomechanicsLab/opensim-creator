#include "simbody_x_opynsim.h"

#include <libopynsim/model_state_stage.h>

#include <liboscar/utilities/enum_helpers.h>
#include <SimTKcommon/internal/Stage.h>

#include <stdexcept>
#include <utility>

using namespace opyn;

opyn::ModelStateStage opyn::to_opynsim_stage(SimTK::Stage simbody_stage)
{
    static_assert(SimTK::Stage::HighestValid == SimTK::Stage::Infinity);
    static_assert(osc::num_options<ModelStateStage>() == 9);

    switch (simbody_stage) {
    case SimTK::Stage::Topology:     return ModelStateStage::topology;
    case SimTK::Stage::Model:        return ModelStateStage::model;
    case SimTK::Stage::Instance:     return ModelStateStage::instance;
    case SimTK::Stage::Time:         return ModelStateStage::time;
    case SimTK::Stage::Position:     return ModelStateStage::position;
    case SimTK::Stage::Velocity:     return ModelStateStage::velocity;
    case SimTK::Stage::Dynamics:     return ModelStateStage::dynamics;
    case SimTK::Stage::Acceleration: return ModelStateStage::acceleration;
    case SimTK::Stage::Report:       return ModelStateStage::report;

    // These are unhandled stages
    case SimTK::Stage::Empty:        [[fallthrough]];

    // Throw a user-facing runtime error if an invalid/unsupported
    // stage somehow slipped through cracks in the OPynSim API.
    default: {
        throw std::runtime_error{"Internal Simbody state stage is incompatible with the OPynSim API: this is a developer error that needs to be reported (preferably, with a bug reproduction, please!)"};
    }
    }
}

SimTK::Stage opyn::to_simbody_stage(ModelStateStage model_state_stage)
{
    static_assert(SimTK::Stage::HighestValid == SimTK::Stage::Infinity);
    static_assert(osc::num_options<ModelStateStage>() == 9);

    switch (model_state_stage) {
    case ModelStateStage::topology:     return SimTK::Stage::Topology;
    case ModelStateStage::model:        return SimTK::Stage::Model;
    case ModelStateStage::instance:     return SimTK::Stage::Instance;
    case ModelStateStage::time:         return SimTK::Stage::Time;
    case ModelStateStage::position:     return SimTK::Stage::Position;
    case ModelStateStage::velocity:     return SimTK::Stage::Velocity;
    case ModelStateStage::dynamics:     return SimTK::Stage::Dynamics;
    case ModelStateStage::acceleration: return SimTK::Stage::Acceleration;
    case ModelStateStage::report:       return SimTK::Stage::Report;
    default:                            std::unreachable();
    }
}
