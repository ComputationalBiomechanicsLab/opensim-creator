#include "single_state_simulation.h"

#include <libopensimcreator/documents/model/basic_model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/param_block.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/utilities/synchronized_value.h>
#include <liboscar/utilities/synchronized_value_guard.h>

using namespace osc;

class osc::SingleStateSimulation::Impl final {
public:
    explicit Impl(BasicModelStatePairWithSharedEnvironment model_state) :
        model_state_{std::move(model_state)}
    {}

    SynchronizedValueGuard<const OpenSim::Model> get_model() const
    {
        return model_state_.lock_child<OpenSim::Model>(
            [](const BasicModelStatePairWithSharedEnvironment& ms) -> const OpenSim::Model& {
                return ms.get_model();
            });
    }

    ptrdiff_t get_num_reports() const { return 0; }

    SimulationReport get_simulation_report(ptrdiff_t) const
    {
        throw std::runtime_error{"invalid method call on a SingleStateSimulation"};
    }

    std::vector<SimulationReport> get_all_simulation_reports() const { return {}; }

    SimulationStatus get_status() const { return SimulationStatus::Completed; }

    SimulationClocks get_clocks() const { return SimulationClocks{SimulationClock::start()}; }

    const ParamBlock& get_params() const { return params_; }

    std::span<const opyn::SharedOutputExtractor> get_output_extractors() const { return {}; }

    float get_fixup_scale_factor() const { return model_state_.lock()->get_fixup_scale_factor(); }

    void set_fixup_scale_factor(float v) { model_state_.lock()->set_fixup_scale_factor(v); }

    std::shared_ptr<Environment> impl_upd_associated_environment()
    {
        return model_state_.lock()->try_upd_environment();
    }

private:
    SynchronizedValue<BasicModelStatePairWithSharedEnvironment> model_state_;
    ParamBlock params_;
};

osc::SingleStateSimulation::SingleStateSimulation(
    BasicModelStatePairWithSharedEnvironment model_state) :

    impl_{std::make_unique<Impl>(std::move(model_state))}
{}
osc::SingleStateSimulation::SingleStateSimulation(SingleStateSimulation&&) noexcept = default;
osc::SingleStateSimulation& osc::SingleStateSimulation::operator=(
    SingleStateSimulation&&) noexcept = default;
osc::SingleStateSimulation::~SingleStateSimulation() noexcept = default;

SynchronizedValueGuard<const OpenSim::Model> osc::SingleStateSimulation::impl_get_model() const
{
    return impl_->get_model();
}

ptrdiff_t osc::SingleStateSimulation::impl_get_num_reports() const
{
    return impl_->get_num_reports();
}

SimulationReport osc::SingleStateSimulation::impl_get_simulation_report(
    ptrdiff_t report_index) const
{
    return impl_->get_simulation_report(report_index);
}

std::vector<SimulationReport> osc::SingleStateSimulation::impl_get_all_simulation_reports() const
{
    return impl_->get_all_simulation_reports();
}

SimulationStatus osc::SingleStateSimulation::impl_get_status() const
{
    return impl_->get_status();
}

SimulationClocks osc::SingleStateSimulation::impl_get_clocks() const
{
    return impl_->get_clocks();
}

const ParamBlock& osc::SingleStateSimulation::impl_get_params() const
{
    return impl_->get_params();
}

std::span<const opyn::SharedOutputExtractor> osc::SingleStateSimulation::
    impl_get_output_extractors() const
{
    return impl_->get_output_extractors();
}

float osc::SingleStateSimulation::impl_get_fixup_scale_factor() const
{
    return impl_->get_fixup_scale_factor();
}

void osc::SingleStateSimulation::impl_set_fixup_scale_factor(float v)
{
    impl_->set_fixup_scale_factor(v);
}

std::shared_ptr<Environment> osc::SingleStateSimulation::impl_upd_associated_environment()
{
    return impl_->impl_upd_associated_environment();
}
