#include "model_state.h"

#include <liboscar/utilities/copy_on_upd_ptr.h>
#include <liboscar/utilities/enum_helpers.h>
#include <SimTKcommon/internal/State.h>

#include <stdexcept>
#include <utility>

using namespace opyn;

namespace
{
    ModelStateStage to_opynsim_stage(SimTK::Stage simbody_stage)
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
}


class opyn::ModelState::Impl final {
public:
    Impl() = default;

    explicit Impl(SimTK::State&& state) : state_{std::move(state)} {}

    ModelStateStage stage() const { return to_opynsim_stage(state_.getSystemStage()); }
    double time() const { return state_.getTime(); }

    const SimTK::State& simbody_state() const { return state_; }
    SimTK::State& simbody_state() { return state_; }
private:
    SimTK::State state_;
};

opyn::ModelState::ModelState() :
    impl_{osc::make_cow<Impl>()}
{}
opyn::ModelState::ModelState(SimTK::State&& state) :
    impl_{osc::make_cow<Impl>(std::move(state))}
{}
opyn::ModelStateStage opyn::ModelState::stage() const { return impl_->stage(); }
double opyn::ModelState::time() const { return impl_->time(); }
const SimTK::State& opyn::ModelState::simbody_state() const { return impl_->simbody_state(); }
SimTK::State& opyn::ModelState::simbody_state() { return impl_.upd()->simbody_state(); }
