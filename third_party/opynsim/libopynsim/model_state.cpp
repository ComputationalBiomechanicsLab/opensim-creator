#include "model_state.h"

#include <liboscar/utilities/copy_on_upd_ptr.h>
#include <SimTKcommon/internal/State.h>

#include <utility>

class opyn::ModelState::Impl final {
public:
    explicit Impl(SimTK::State&& state) : state_{std::move(state)} {}

    const SimTK::State& simbody_state() const { return state_; }
private:
    SimTK::State state_;
};

opyn::ModelState::ModelState(SimTK::State&& state) :
    impl_{osc::make_cow<Impl>(std::move(state))}
{}
const SimTK::State& opyn::ModelState::simbody_state() const { return impl_->simbody_state(); }
