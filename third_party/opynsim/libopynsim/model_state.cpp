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

    const attrs_type& attrs() const { return attrs_; }
    void set_attrs(attrs_type new_attrs) { attrs_ = std::move(new_attrs); }

    const SimTK::State& simbody_state() const { return state_; }
    SimTK::State& upd_simbody_state() { return state_; }
private:
    SimTK::State state_;
    attrs_type attrs_;
};

opyn::ModelState::ModelState() :
    impl_{osc::make_cow<Impl>()}
{}
opyn::ModelState::ModelState(SimTK::State&& state) :
    impl_{osc::make_cow<Impl>(std::move(state))}
{}
opyn::ModelStateStage opyn::ModelState::stage() const { return impl_->stage(); }
double opyn::ModelState::time() const { return impl_->time(); }
const opyn::ModelState::attrs_type& opyn::ModelState::attrs() const { return impl_->attrs(); }
void opyn::ModelState::set_attrs(attrs_type new_attrs) { impl_.upd()->set_attrs(std::move(new_attrs)); }
const SimTK::State& opyn::ModelState::simbody_state() const { return impl_->simbody_state(); }
SimTK::State& opyn::ModelState::upd_simbody_state() { return impl_.upd()->upd_simbody_state(); }
