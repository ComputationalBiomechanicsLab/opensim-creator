#include "simulation_report.h"

#include <SimTKcommon/internal/State.h>

#include <liboscar/utilities/algorithms.h>

#include <chrono>
#include <memory>
#include <unordered_map>
#include <utility>

using namespace osc;

class osc::SimulationReport::Impl final {
public:
    Impl() = default;

    explicit Impl(SimTK::State&& st) :
        state_{std::move(st)}
    {}

    Impl(SimTK::State&& st, std::unordered_map<UID, float> auxiliary_values) :
        state_{std::move(st)},
        auxiliary_values_{std::move(auxiliary_values)}
    {}

    std::unique_ptr<Impl> clone() const { return std::make_unique<Impl>(*this); }

    SimulationClock::time_point get_time() const
    {
        return SimulationClock::start() + SimulationClock::duration{get_state().getTime()};
    }

    const SimTK::State& get_state() const { return state_; }

    SimTK::State& upd_state_hack() { return state_; }

    std::optional<float> get_auxiliary_value(UID id) const
    {
        return lookup_or_nullopt(auxiliary_values_, id);
    }

private:
    SimTK::State state_;
    std::unordered_map<UID, float> auxiliary_values_;
};

osc::SimulationReport::SimulationReport() :
    impl_{std::make_shared<Impl>()}
{}
osc::SimulationReport::SimulationReport(SimTK::State&& st) :
    impl_{std::make_shared<Impl>(std::move(st))}
{}
osc::SimulationReport::SimulationReport(
    SimTK::State&& st, std::unordered_map<UID, float> auxiliary_values) :

    impl_{std::make_shared<Impl>(std::move(st), std::move(auxiliary_values))}
{}
osc::SimulationReport::SimulationReport(const SimulationReport&) = default;
osc::SimulationReport::SimulationReport(SimulationReport&&) noexcept = default;
osc::SimulationReport& osc::SimulationReport::operator=(const SimulationReport&) = default;
osc::SimulationReport& osc::SimulationReport::operator=(SimulationReport&&) noexcept = default;
osc::SimulationReport::~SimulationReport() noexcept = default;

SimulationClock::time_point osc::SimulationReport::get_time() const
{
    return impl_->get_time();
}

const SimTK::State& osc::SimulationReport::impl_get_state() const
{
    return impl_->get_state();
}

SimTK::State& osc::SimulationReport::upd_state_hack()
{
    return impl_->upd_state_hack();
}

std::optional<float> osc::SimulationReport::impl_get_auxiliary_value(UID id) const
{
    return impl_->get_auxiliary_value(id);
}
