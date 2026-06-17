#include "model_state.h"

#include <libopynsim/utilities/simbody_x_opynsim.h>

#include <liboscar/utilities/copy_on_upd_ptr.h>
#include <SimTKcommon/internal/State.h>

#include <utility>

using namespace opyn;

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
