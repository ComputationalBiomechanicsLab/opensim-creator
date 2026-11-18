#include "simulation_model_state_pair.h"

#include <libopensimcreator/documents/model/basic_model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/simulation/simulation.h>
#include <libopensimcreator/documents/simulation/simulation_report.h>
#include <libopensimcreator/documents/simulation/single_state_simulation.h>

#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/utilities/uid.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <utility>

using namespace osc;

class osc::SimulationModelStatePair::Impl final {
public:
    Impl() :
        simulation_{std::make_shared<Simulation>(
            SingleStateSimulation{BasicModelStatePairWithSharedEnvironment{}})}
    {}

    Impl(std::shared_ptr<Simulation> simulation, SimulationReport simulation_report) :
        simulation_{std::move(simulation)},
        simulation_report_{std::move(simulation_report)}
    {}

    const OpenSim::Model& get_model() const
    {
        return *simulation_->get_model();  // TODO: UH OH - lock leak (#707 - are locks necessary?)
    }

    UID get_model_version() const { return model_version_; }

    const SimTK::State& get_state() const { return simulation_report_.state(); }

    UID get_state_version() const { return state_version_; }

    const OpenSim::Component* get_selected() const
    {
        return opyn::find_component(get_model(), selected_);
    }

    void set_selected(const OpenSim::Component* c)
    {
        selected_ = opyn::get_absolute_path_or_empty(c);
    }

    const OpenSim::Component* get_hovered() const
    {
        return opyn::find_component(get_model(), hovered_);
    }

    void set_hovered(const OpenSim::Component* c)
    {
        hovered_ = opyn::get_absolute_path_or_empty(c);
    }

    float get_fixup_scale_factor() const { return simulation_->get_fixup_scale_factor(); }

    void set_fixup_scale_factor(float v) { simulation_->set_fixup_scale_factor(v); }

    std::shared_ptr<Environment> impl_upd_associated_environment() const
    {
        return simulation_->try_upd_environment();
    }

    std::shared_ptr<Simulation> upd_simulation() { return simulation_; }

    void set_simulation(std::shared_ptr<Simulation> s)
    {
        if (s != simulation_) {
            simulation_ = std::move(s);
            model_version_ = UID{};
        }
    }

    SimulationReport get_simulation_report() const { return simulation_report_; }

    void set_simulation_report(SimulationReport r)
    {
        if (r != simulation_report_) {
            simulation_report_ = std::move(r);
            state_version_ = UID{};
        }
    }

private:
    UID model_version_;
    UID state_version_;
    OpenSim::ComponentPath selected_;
    OpenSim::ComponentPath hovered_;
    std::shared_ptr<Simulation> simulation_;
    SimulationReport simulation_report_;
};

osc::SimulationModelStatePair::SimulationModelStatePair() :
    impl_{std::make_unique<Impl>()}
{}

osc::SimulationModelStatePair::SimulationModelStatePair(
    std::shared_ptr<Simulation> simulation, SimulationReport report) :

    impl_{std::make_unique<Impl>(std::move(simulation), std::move(report))}
{}

osc::SimulationModelStatePair::SimulationModelStatePair(SimulationModelStatePair&&) noexcept =
    default;
osc::SimulationModelStatePair& osc::SimulationModelStatePair::operator=(
    SimulationModelStatePair&&) noexcept = default;
osc::SimulationModelStatePair::~SimulationModelStatePair() noexcept = default;

std::shared_ptr<Simulation> osc::SimulationModelStatePair::upd_simulation()
{
    return impl_->upd_simulation();
}

void osc::SimulationModelStatePair::set_simulation(std::shared_ptr<Simulation> sim)
{
    impl_->set_simulation(std::move(sim));
}

SimulationReport osc::SimulationModelStatePair::get_simulation_report() const
{
    return impl_->get_simulation_report();
}

void osc::SimulationModelStatePair::set_simulation_report(SimulationReport report)
{
    impl_->set_simulation_report(std::move(report));
}

const OpenSim::Model& osc::SimulationModelStatePair::impl_get_model() const
{
    return impl_->get_model();
}

UID osc::SimulationModelStatePair::impl_get_model_version() const
{
    return impl_->get_model_version();
}

const SimTK::State& osc::SimulationModelStatePair::impl_get_state() const
{
    return impl_->get_state();
}

UID osc::SimulationModelStatePair::impl_get_state_version() const
{
    return impl_->get_state_version();
}

const OpenSim::Component* osc::SimulationModelStatePair::impl_get_selected() const
{
    return impl_->get_selected();
}

void osc::SimulationModelStatePair::impl_set_selected(const OpenSim::Component* c)
{
    impl_->set_selected(c);
}

const OpenSim::Component* osc::SimulationModelStatePair::impl_get_hovered() const
{
    return impl_->get_hovered();
}

void osc::SimulationModelStatePair::impl_set_hovered(const OpenSim::Component* c)
{
    impl_->set_hovered(c);
}

float osc::SimulationModelStatePair::impl_get_fixup_scale_factor() const
{
    return impl_->get_fixup_scale_factor();
}

void osc::SimulationModelStatePair::impl_set_fixup_scale_factor(float v)
{
    impl_->set_fixup_scale_factor(v);
}

std::shared_ptr<Environment> osc::SimulationModelStatePair::impl_upd_associated_environment() const
{
    return impl_->impl_upd_associated_environment();
}
