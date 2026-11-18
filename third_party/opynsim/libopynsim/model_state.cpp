#include "model_state.h"

#include <libopynsim/utilities/simbody_x_opynsim.h>

#include <liboscar/maths/common_functions.h>
#include <liboscar/utilities/copy_on_upd_ptr.h>
#include <SimTKcommon/internal/State.h>
#include <SimTKcommon/internal/Vector_.h>

#include <algorithm>
#include <span>
#include <utility>

using namespace opyn;
namespace rgs = std::ranges;

namespace
{
    template<typename T>
    std::span<const T> to_span(const SimTK::Vector_<T>& v)
    {
        return {&v[0], static_cast<std::span<const T>::size_type>(v.size())};
    }

    template<typename T>
    bool have_equivalent_memory(const SimTK::Vector_<T>& lhs, const SimTK::Vector_<T>& rhs)
    {
        return rgs::equal(as_bytes(to_span(lhs)), as_bytes(to_span(rhs)));
    }

    bool are_equivalent(const SimTK::State& lhs, const SimTK::State& rhs)
    {
        if (not osc::equal_within_absdiff(lhs.getTime(), rhs.getTime(), osc::epsilon_v<double>)) {
            return false;  // Different times
        }
        if (lhs.getSystemStage() != rhs.getSystemStage()) {
            return false;  // Different stages
        }
        if (lhs.getSystemStage() < SimTK::Stage::Time) {
            // Same time+stage, but no state variables allocated: nothing more to compare.
            return true;
        }

        // clang-format off
        return  have_equivalent_memory(lhs.getQ(),           rhs.getQ())
            and have_equivalent_memory(lhs.getU(),           rhs.getU())
            and have_equivalent_memory(lhs.getZ(),           rhs.getZ())
            and have_equivalent_memory(lhs.getUWeights(),    rhs.getUWeights())
            and have_equivalent_memory(lhs.getZWeights(),    rhs.getZWeights())
            and have_equivalent_memory(lhs.getQErrWeights(), rhs.getQErrWeights())
            and have_equivalent_memory(lhs.getUErrWeights(), rhs.getUErrWeights());
        // clang-format on
    }
}

class opyn::ModelState::Impl final {
public:
    Impl() = default;

    explicit Impl(SimTK::State&& state) :
        state_{std::move(state)}
    {}

    friend bool operator==(const Impl& lhs, const Impl& rhs)
    {
        return are_equivalent(lhs.state_, rhs.state_) and lhs.attrs_ == rhs.attrs_;
    }

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
opyn::ModelStateStage opyn::ModelState::stage() const
{
    return impl_->stage();
}
double opyn::ModelState::time() const
{
    return impl_->time();
}
const opyn::ModelState::attrs_type& opyn::ModelState::attrs() const
{
    return impl_->attrs();
}
void opyn::ModelState::set_attrs(attrs_type new_attrs)
{
    impl_.upd()->set_attrs(std::move(new_attrs));
}
const SimTK::State& opyn::ModelState::simbody_state() const
{
    return impl_->simbody_state();
}
SimTK::State& opyn::ModelState::upd_simbody_state()
{
    return impl_.upd()->upd_simbody_state();
}
bool opyn::operator==(const ModelState& lhs, const ModelState& rhs)
{
    return lhs.impl_ == rhs.impl_ or *lhs.impl_ == *rhs.impl_;
}
